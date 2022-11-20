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
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <nlohmann/json.hpp>

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

        static nlohmann::json connectionJson;
        static nlohmann::json sharedMappingJson;

        if (mappingJson.contains("connection")) {
            connectionJson = mappingJson["connection"];
        }

        if (mappingJson.contains("discover_prefix") && mappingJson["discover_prefix"].is_string() &&
            mappingJson["discover_prefix"] == discoverPrefix) {
            if (mappingJson.contains("mappings") && mappingJson["mappings"].is_object() &&
                mappingJson["mappings"].contains("topic_level")) {
                sharedMappingJson = mappingJson["mappings"];

                using InMqttIntegratorClient = net::in::stream::legacy::SocketClient<
                    apps::mqttbroker::integrator::SocketContextFactory<connectionJson, sharedMappingJson>>;

                using LegacyInSocketConnection = InMqttIntegratorClient::SocketConnection;

                decltype(
                    [](const InMqttIntegratorClient& inMqttIntegratorClient, const std::function<void()>& stopTimer = nullptr) -> void {
                        inMqttIntegratorClient.connect(
                            [stopTimer](const InMqttIntegratorClient::SocketAddress& socketAddress, int errnum) -> void {
                                if (errnum != 0) {
                                    PLOG(ERROR) << "OnError: " << socketAddress.toString();
                                } else {
                                    VLOG(0) << "MqttIntegrator connected to " << socketAddress.toString();

                                    if (stopTimer) {
                                        stopTimer();
                                    }
                                }
                            });
                    }) doConnect{};

                InMqttIntegratorClient inMqttIntegratorClient(
                    "clientmapper",
                    [](LegacyInSocketConnection* socketConnection) -> void {
                        VLOG(0) << "OnConnect";

                        VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                        VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
                    },
                    []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void {
                        VLOG(0) << "OnConnected";
                    },
                    [&doConnect, &inMqttIntegratorClient](LegacyInSocketConnection* socketConnection) -> void {
                        VLOG(0) << "OnDisconnect";

                        VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                        VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

                        core::timer::Timer timer = core::timer::Timer::intervalTimer(
                            [&doConnect, &inMqttIntegratorClient](const std::function<void()>& stop) -> void {
                                doConnect(inMqttIntegratorClient, stop);
                            },
                            1);
                    });

                doConnect(inMqttIntegratorClient);

                ret = core::SNodeC::start();
            } else {
                LOG(ERROR) << "Mapping object \"mapping\" not found in " << mappingFilePath;

                ret = 2;
            }
        } else {
            LOG(ERROR) << "Discover prefix object \"" << discoverPrefix << "\" not found in" << mappingFilePath;

            ret = 1;
        }
    } else {
        ret = core::SNodeC::start();
    }

    return ret;
}
