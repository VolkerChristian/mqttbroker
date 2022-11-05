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

#include "../JsonMappingReader.h"
#include "SocketContext.h"          // IWYU pragma: keep
#include "SocketContextFactory.hpp" // IWYU pragma: keep
#include "core/SNodeC.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <nlohmann/json.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

int main(int argc, char* argv[]) {
    std::string mappingFilePath;
    utils::Config::add_option(
        "--mqtt-mapping-file", mappingFilePath, "MQTT mapping file (json format) for integration", true, "[path to json file]");

    std::string discoverPrefix;
    utils::Config::add_option(
        "--mqtt-discover-prefix", discoverPrefix, "MQTT discover prefix in the json mapping file", false, "[utf8]", "iotempower");

    core::SNodeC::init(argc, argv);

    static const nlohmann::json& jsonMapping = apps::mqttbroker::JsonMappingReader::readMappingFromFile(mappingFilePath);

    static nlohmann::json sharedJsonMapping;

    if (jsonMapping.contains(discoverPrefix)) {
        sharedJsonMapping = jsonMapping[discoverPrefix];
    }

    using ClientBrokerInServer = net::in::stream::legacy::SocketClient<apps::mqttbroker::SocketContextFactory<sharedJsonMapping>>;

    using LegacyInSocketConnection = ClientBrokerInServer::SocketConnection;

    ClientBrokerInServer clientBrokerInServer(
        "clientmapper",
        [](LegacyInSocketConnection* socketConnection) -> void {
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        },
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void {
            VLOG(0) << "OnConnected";
        },
        [](LegacyInSocketConnection* socketConnection) -> void {
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

    clientBrokerInServer.connect([](const ClientBrokerInServer::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            //            errno = errnum;
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c connecting to " << socketAddress.toString();
        }
    });

    core::SNodeC::start();
}
