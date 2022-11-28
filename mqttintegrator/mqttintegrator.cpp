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

#include "SocketContext.h"          // IWYU pragma: keep
#include "SocketContextFactory.hpp" // IWYU pragma: keep
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "lib/JsonMappingReader.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/tls/SocketClient.h"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <openssl/opensslv.h>
#include <openssl/ssl3.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>
#endif
#include <nlohmann/json.hpp>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

int main(int argc, char* argv[]) {
    int ret = 0;

    std::string mappingFilePath;
    utils::Config::add_option(
        "--mqtt-mapping-file", mappingFilePath, "MQTT mapping file (json format) for integration", true, "[path to json file]");

    std::string discoverPrefix;
    utils::Config::add_option(
        "--mqtt-discover-prefix", discoverPrefix, "MQTT discover prefix in the json mapping file", false, "[utf8]", "iotempower");

    core::SNodeC::init(argc, argv);

    if (!mappingFilePath.empty()) {
        const nlohmann::json& mappingJson = apps::mqttbroker::lib::JsonMappingReader::readMappingFromFile(mappingFilePath);

        if (!mappingJson.empty()) {
            static nlohmann::json connectionJson;
            static nlohmann::json sharedMappingJson;

            if (mappingJson.contains("connection")) {
                connectionJson = mappingJson["connection"];
            }

            sharedMappingJson = mappingJson["mappings"];

            using InMqttTlsIntegratorClient =
                net::in::stream::tls::SocketClient<apps::mqttbroker::integrator::SocketContextFactory<connectionJson, sharedMappingJson>>;

            using TLSInSocketConnection = InMqttTlsIntegratorClient::SocketConnection;

            decltype(
                [](const InMqttTlsIntegratorClient& inMqttTlsIntegratorClient, const std::function<void()>& stopTimer = nullptr) -> void {
                    inMqttTlsIntegratorClient.connect(
                        [stopTimer](const TLSInSocketConnection::SocketAddress& socketAddress, int errnum) -> void {
                            if (errnum != 0) {
                                PLOG(ERROR) << "OnError: " << socketAddress.toString();
                            } else {
                                VLOG(0) << "MqttIntegrator connected to " << socketAddress.toString();

                                if (stopTimer) {
                                    stopTimer();
                                }
                            }
                        });
                }) doConnect;

            InMqttTlsIntegratorClient inMqttTlsIntegratorClient(
                "mqtttlsintegrator",
                [](TLSInSocketConnection* socketConnection) -> void {
                    VLOG(0) << "OnConnect";

                    VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                    VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

                    // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

                    /* Enable automatic hostname checks */
                    // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
                    // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
                    //     // handle error
                    //     socketConnection->close();
                    // }
                },
                []([[maybe_unused]] TLSInSocketConnection* socketConnection) -> void {
                    VLOG(0) << "OnConnected";

                    X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                    if (server_cert != nullptr) {
                        long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                        VLOG(0) << "\tPeer certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                        if (verifyErr == X509_V_OK) {
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
                                        std::string subjectAltName = std::string(
                                            reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
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
                        } else {
                            socketConnection->close();
                        }

                        X509_free(server_cert);
                    } else {
                        VLOG(0) << "\tPeer certificate: no certificate";
                        // Here we can close the connection in case client didn't send a certificate
                    }
                },
                [&doConnect, &inMqttTlsIntegratorClient](TLSInSocketConnection* socketConnection) -> void {
                    VLOG(0) << "OnDisconnect";

                    VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                    VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

                    core::timer::Timer timer = core::timer::Timer::intervalTimer(
                        [&doConnect, &inMqttTlsIntegratorClient](const std::function<void()>& stop) -> void {
                            doConnect(inMqttTlsIntegratorClient, stop);
                        },
                        1);
                });

            doConnect(inMqttTlsIntegratorClient);

            ret = core::SNodeC::start();
        }
    } else {
        ret = core::SNodeC::start();
    }

    return ret;
}
