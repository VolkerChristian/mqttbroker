/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MqttModel.h"
#include "SharedSocketContextFactory.h" // IWYU pragma: keep
#include "lib/Mqtt.h"

namespace iot::mqtt::packets {
    class Connect;
}

#include <core/SNodeC.h>
#include <core/socket/SocketAddress.h>
#include <express/legacy/in/WebApp.h>
#include <express/tls/in/WebApp.h>
#include <log/Logger.h>
#include <net/in/stream/legacy/SocketServer.h> // IWYU pragma: keep
#include <net/in/stream/tls/SocketServer.h>    // IWYU pragma: keep
#include <net/un/stream/legacy/SocketServer.h> // IWYU pragma: keep
#include <utils/Config.h>
#include <web/http/http_utils.h>

//

#include <nlohmann/json.hpp>
#include <openssl/asn1.h>
#include <openssl/crypto.h>
#include <openssl/obj_mac.h>
#include <openssl/opensslv.h>
#include <openssl/ssl3.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>
#endif
#include <cstdlib>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

int main(int argc, char* argv[]) {
    std::string mappingFilePath;
    utils::Config::add_option(
        "--mqtt-mapping-file", mappingFilePath, "MQTT mapping file (json format) for integration", false, "[path to json file]");

    core::SNodeC::init(argc, argv);

    static nlohmann::json sharedJsonMapping;

    if (!mappingFilePath.empty()) {
        setenv("MQTT_MAPPING_FILE", mappingFilePath.data(), 0);
    }

    using MQTTLegacyInServer = net::in::stream::legacy::SocketServer<mqttbroker::broker::SharedSocketContextFactory>;

    using LegacyInSocketConnection = MQTTLegacyInServer::SocketConnection;

    MQTTLegacyInServer mqttLegacyInServer(
        "legacyin",
        [](LegacyInSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
        },
        [](LegacyInSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        [](LegacyInSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        });

    mqttLegacyInServer.listen([mqttLegacyInServer](const MQTTLegacyInServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError";
        } else {
            VLOG(0) << mqttLegacyInServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    using MQTTTLSInServer = net::in::stream::tls::SocketServer<mqttbroker::broker::SharedSocketContextFactory>;
    using TLSInSocketConnection = MQTTTLSInServer::SocketConnection;

    MQTTTLSInServer mqttTLSInServer(
        "tlsin",
        [](TLSInSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";
            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        [](TLSInSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";
            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != nullptr) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                VLOG(0) << "\tPeer certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                VLOG(0) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                VLOG(0) << "\t   Issuer: " + std::string(str);
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));

                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                if (altNameCount > 0) {
                    VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                        if (generalName->type == GEN_URI) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                            VLOG(0) << "\t      SAN (URI): '" + subjectAltName;
                        } else if (generalName->type == GEN_DNS) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                            VLOG(0) << "\t      SAN (DNS): '" + subjectAltName;
                        } else {
                            VLOG(0) << "\t      SAN (Type): '" + std::to_string(generalName->type);
                        }
                    }
                }

                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                X509_free(server_cert);
            } else {
                VLOG(0) << "\tPeer certificate: no certificate";
                // Here we can close the connection in case client didn't send a certificate
            }
        },
        [](TLSInSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        });

    //    std::map<std::string, std::map<std::string, std::any>> sniCerts = {
    //        {"snodec.home.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}},
    //        {"www.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}}};

    //    mqttTLSInServer.addSniCerts(sniCerts);

    mqttTLSInServer.listen([mqttTLSInServer](const MQTTTLSInServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError";
        } else {
            VLOG(0) << mqttTLSInServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    using MQTTLegacyUnServer = net::un::stream::legacy::SocketServer<mqttbroker::broker::SharedSocketContextFactory>;
    using LegacyUnSocketConnection = MQTTLegacyUnServer::SocketConnection;

    MQTTLegacyUnServer mqttLegacyUnServer(
        "legacyun",
        [](LegacyUnSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        [](LegacyUnSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        [](LegacyUnSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        });

    mqttLegacyUnServer.listen(
        [mqttLegacyUnServer](const LegacyUnSocketConnection::SocketAddress& socketAddress, int errnum) mutable -> void {
            if (errnum < 0) {
                PLOG(ERROR) << "OnError";
            } else if (errnum > 0) {
                PLOG(ERROR) << "OnError";
            } else {
                VLOG(0) << mqttLegacyUnServer.getConfig().getName() << " listening on " << socketAddress.toString();
            }
        });

    express::tls::in::WebApp mqttWebView("mqttwebview");

    //    mqttWebView.laxRouting();

    mqttWebView.get("/test", [] APPLICATION(req, res) {
        res.send("Response From MQTTWebView");
    });

    mqttWebView.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqttbroker::broker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqttbroker::broker::lib::MqttModel::instance().getConnectedClinets();

        std::string responseString = "<html>"
                                     "  <head>"
                                     "    <title>Response from MqttWebFrontend</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>List of all Connected Clients</h1>"
                                     "    <table>"
                                     "      <tr><th>ClientId</th><th>Address</th></tr>";

        for (const auto& [mqtt, connectPacket] : connectionList) {
            responseString +=
                "<tr><td>" + mqtt->getClientId() + "</td><td>" + mqtt->getSocketConnection()->getRemoteAddress().toString() + "</td></tr>";
        }

        responseString += "    </table>"
                          "  </body>"
                          "</html>";

        res.send(responseString);
    });

    mqttWebView.get("/ws/", [] APPLICATION(req, res) -> void {
        std::string uri = req.originalUrl;

        VLOG(0) << "OriginalUri: " << uri;
        VLOG(0) << "Uri: " << req.url;

        VLOG(0) << "Host: " << req.get("host");
        VLOG(0) << "Connection: " << req.get("connection");
        VLOG(0) << "Origin: " << req.get("origin");
        VLOG(0) << "Path: " << req.path;
        VLOG(0) << "Sec-WebSocket-Protocol: " << req.get("sec-websocket-protocol");
        VLOG(0) << "sec-web-socket-extensions: " << req.get("sec-websocket-extensions");
        VLOG(0) << "sec-websocket-key: " << req.get("sec-websocket-key");
        VLOG(0) << "sec-websocket-version: " << req.get("sec-websocket-version");
        VLOG(0) << "upgrade: " << req.get("upgrade");
        VLOG(0) << "user-agent: " << req.get("user-agent");

        if (httputils::ci_contains(req.get("connection"), "Upgrade")) {
            res.upgrade(req);
        } else {
            res.sendStatus(404);
        }
    });

    mqttWebView.listen([](const express::tls::in::WebApp::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError";
        } else {
            VLOG(0) << "MqttWebFrontend listening on " << socketAddress.toString();
        }
    });

    express::legacy::in::WebApp mqttLegacyWebView("mqttlegacywebview");

    //    mqttLegacyWebView.laxRouting();

    mqttLegacyWebView.get("/test", [] APPLICATION(req, res) {
        res.send("Response From MQTTWebView");
    });

    mqttLegacyWebView.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqttbroker::broker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqttbroker::broker::lib::MqttModel::instance().getConnectedClinets();

        std::string responseString = "<html>"
                                     "  <head>"
                                     "    <title>Response from MqttWebFrontend</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>List of all Connected Clients</h1>"
                                     "    <table>"
                                     "      <tr><th>ClientId</th><th>Address</th></tr>";

        for (const auto& [mqtt, connectPacket] : connectionList) {
            responseString +=
                "<tr><td>" + mqtt->getClientId() + "</td><td>" + mqtt->getSocketConnection()->getRemoteAddress().toString() + "</td></tr>";
        }

        responseString += "    </table>"
                          "  </body>"
                          "</html>";

        res.send(responseString);
    });

    mqttLegacyWebView.get("/ws/", [] APPLICATION(req, res) -> void {
        std::string uri = req.originalUrl;

        VLOG(0) << "OriginalUri: " << uri;
        VLOG(0) << "Uri: " << req.url;

        VLOG(0) << "Host: " << req.get("host");
        VLOG(0) << "Connection: " << req.get("connection");
        VLOG(0) << "Origin: " << req.get("origin");
        VLOG(0) << "Path: " << req.path;
        VLOG(0) << "Sec-WebSocket-Protocol: " << req.get("sec-websocket-protocol");
        VLOG(0) << "sec-web-socket-extensions: " << req.get("sec-websocket-extensions");
        VLOG(0) << "sec-websocket-key: " << req.get("sec-websocket-key");
        VLOG(0) << "sec-websocket-version: " << req.get("sec-websocket-version");
        VLOG(0) << "upgrade: " << req.get("upgrade");
        VLOG(0) << "user-agent: " << req.get("user-agent");

        if (httputils::ci_contains(req.get("connection"), "Upgrade")) {
            res.upgrade(req);
        } else {
            res.sendStatus(404);
        }
    });

    mqttLegacyWebView.get("/", [] APPLICATION(req, res) -> void {
        std::string uri = req.originalUrl;

        VLOG(0) << "OriginalUri: " << uri;
        VLOG(0) << "Uri: " << req.url;

        VLOG(0) << "Host: " << req.get("host");
        VLOG(0) << "Connection: " << req.get("connection");
        VLOG(0) << "Origin: " << req.get("origin");
        VLOG(0) << "Path: " << req.path;
        VLOG(0) << "Sec-WebSocket-Protocol: " << req.get("sec-websocket-protocol");
        VLOG(0) << "sec-web-socket-extensions: " << req.get("sec-websocket-extensions");
        VLOG(0) << "sec-websocket-key: " << req.get("sec-websocket-key");
        VLOG(0) << "sec-websocket-version: " << req.get("sec-websocket-version");
        VLOG(0) << "upgrade: " << req.get("upgrade");
        VLOG(0) << "user-agent: " << req.get("user-agent");

        if (httputils::ci_contains(req.get("connection"), "Upgrade")) {
            res.upgrade(req);
        } else {
            res.sendStatus(404);
        }
    });

    mqttLegacyWebView.listen([](const express::legacy::in::WebApp::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError";
        } else {
            VLOG(0) << "MqttWebFrontend listening on " << socketAddress.toString();
        }
    });
    return core::SNodeC::start();
}
