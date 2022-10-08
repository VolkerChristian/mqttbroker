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

#include "broker/SharedSocketContextFactory.h" // IWYU pragma: keep
#include "broker/SocketContextFactory.h"       // IWYU pragma: keep
#include "config.h"                            // just for this example app
#include "core/SNodeC.h"
#include "log/Logger.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/in/stream/tls/SocketServer.h"
#include "net/un/stream/legacy/SocketServer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using MQTTLegacyInServer = net::in::stream::legacy::SocketServer<mqtt::broker::SharedSocketContextFactory>;
    using LegacyInSocketConnection = MQTTLegacyInServer::SocketConnection;

    MQTTLegacyInServer mqttLegacyInServer(
        "legacyin",
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        });

    mqttLegacyInServer.listen([mqttLegacyInServer](const MQTTLegacyInServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << mqttLegacyInServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    using MQTTTLSInServer = net::in::stream::tls::SocketServer<mqtt::broker::SharedSocketContextFactory>;
    using TLSInSocketConnection = MQTTTLSInServer::SocketConnection;

    std::map<std::string, std::any> options{{"CertChain", SERVERCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}};
    std::map<std::string, std::map<std::string, std::any>> sniCerts = {
        {"snodec.home.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}},
        {"www.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}}};

    MQTTTLSInServer mqttTLSInServer(
        "tlsin",
        []([[maybe_unused]] TLSInSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";
            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        []([[maybe_unused]] TLSInSocketConnection* socketConnection) -> void { // OnConnected
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
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                X509_free(server_cert);
            } else {
                VLOG(0) << "\tPeer certificate: no certificate";
                // Here we can close the connection in case client didn't send a certificate
            }
        },
        []([[maybe_unused]] TLSInSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        options);

    mqttTLSInServer.addSniCerts(sniCerts);

    mqttTLSInServer.listen([mqttTLSInServer](const MQTTTLSInServer::SocketAddress& socketAddress, int errnum) mutable -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << mqttTLSInServer.getConfig().getName() << " listening on " << socketAddress.toString();
        }
    });

    using MQTTLegacyUnServer = net::un::stream::legacy::SocketServer<mqtt::broker::SharedSocketContextFactory>;
    using LegacyUnSocketConnection = MQTTLegacyUnServer::SocketConnection;

    MQTTLegacyUnServer mqttLegacyUnServer(
        "legacyun",
        []([[maybe_unused]] LegacyUnSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        []([[maybe_unused]] LegacyUnSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

        },
        []([[maybe_unused]] LegacyUnSocketConnection* socketConnection) -> void { // OnDisconnected
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
                PLOG(ERROR) << "OnError: " << socketAddress.toString();
            } else {
                VLOG(0) << mqttLegacyUnServer.getConfig().getName() << " listening on " << socketAddress.toString();
            }
        });

    return core::SNodeC::start();
}
