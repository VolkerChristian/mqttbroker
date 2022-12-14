/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include <cstdlib>

int main(int argc, char* argv[]) {
    std::string mappingFilePath;
    utils::Config::add_option("--mqtt-mapping-file", mappingFilePath, "MQTT mapping file (json format) for integration", false, "[path]");

    std::string sessionStore;
    utils::Config::add_option("--mqtt-session-store", sessionStore, "Path to file for the persistent session store", false, "[path]");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_MAPPING_FILE", mappingFilePath.data(), 0);
    setenv("MQTT_SESSION_STORE", sessionStore.data(), 0);

    using MQTTLegacyInServer = net::in::stream::legacy::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;

    MQTTLegacyInServer mqttLegacyInServer("legacyin");

    mqttLegacyInServer.listen([mqttLegacyInServer](const MQTTLegacyInServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else if (errnum > 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else {
            VLOG(0) << mqttLegacyInServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    using MQTTTLSInServer = net::in::stream::tls::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;

    MQTTTLSInServer mqttTLSInServer("tlsin");

    //    std::map<std::string, std::map<std::string, std::any>> sniCerts = {
    //        {"snodec.home.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}},
    //        {"www.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}}};

    //    mqttTLSInServer.addSniCerts(sniCerts);

    mqttTLSInServer.listen([mqttTLSInServer](const MQTTTLSInServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else if (errnum > 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else {
            VLOG(0) << mqttTLSInServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    using MQTTLegacyUnServer = net::un::stream::legacy::SocketServer<mqtt::mqttbroker::SharedSocketContextFactory>;

    MQTTLegacyUnServer mqttLegacyUnServer("legacyun");

    mqttLegacyUnServer.listen([mqttLegacyUnServer](const MQTTLegacyUnServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else if (errnum > 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else {
            VLOG(0) << mqttLegacyUnServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    express::tls::in::WebApp mqttTLSWebView("mqtttlswebview");

    mqttTLSWebView.get("/test", [] APPLICATION(req, res) {
        res.send("Response From MQTTWebView");
    });

    mqttTLSWebView.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqtt::mqttbroker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqtt::mqttbroker::lib::MqttModel::instance().getConnectedClinets();

        std::string responseString = "<html>"
                                     "  <head>"
                                     "    <title>Response from MqttWebFrontend</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>List of all Connected Clients</h1>"
                                     "    <table>"
                                     "      <tr><th>ClientId</th><th>Locale Address</th><th>Remote Address</th></tr>";

        for (const auto& [mqtt, connectPacket] : connectionList) {
            responseString += "<tr><td>" + mqtt->getClientId() + "</td><td>" + mqtt->getSocketConnection()->getLocalAddress().toString() +
                              "</td><td>" + mqtt->getSocketConnection()->getRemoteAddress().toString() + "</td></tr>";
        }

        responseString += "    </table>"
                          "  </body>"
                          "</html>";

        res.send(responseString);
    });

    mqttTLSWebView.get("/ws/", [] APPLICATION(req, res) -> void {
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

    mqttTLSWebView.listen([](const express::tls::in::WebApp::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else if (errnum > 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else {
            VLOG(0) << "MqttWebFrontend listening on " << socketAddress.toString();
        }
    });

    express::legacy::in::WebApp mqttLegacyWebView("mqttlegacywebview");

    mqttLegacyWebView.get("/test", [] APPLICATION(req, res) {
        res.send("Response From MQTTWebView");
    });

    mqttLegacyWebView.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqtt::mqttbroker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqtt::mqttbroker::lib::MqttModel::instance().getConnectedClinets();

        std::string responseString = "<html>"
                                     "  <head>"
                                     "    <title>Response from MqttWebFrontend</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>List of all Connected Clients</h1>"
                                     "    <table>"
                                     "      <tr><th>ClientId</th><th>Locale Address</th><th>Remote Address</th></tr>";

        for (const auto& [mqtt, connectPacket] : connectionList) {
            responseString += "<tr><td>" + mqtt->getClientId() + "</td><td>" + mqtt->getSocketConnection()->getLocalAddress().toString() +
                              "</td><td>" + mqtt->getSocketConnection()->getRemoteAddress().toString() + "</td></tr>";
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
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else if (errnum > 0) {
            PLOG(ERROR) << "listening on " << socketAddress.toString();
        } else {
            VLOG(0) << "MqttWebFrontend listening on " << socketAddress.toString();
        }
    });

    return core::SNodeC::start();
}
