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

#ifndef APPS_MQTTBROKER_BROKER_SOCKETCONTEXTFACTORY_H
#define APPS_MQTTBROKER_BROKER_SOCKETCONTEXTFACTORY_H

namespace mqttbroker::broker {
    class Mqtt;
} // namespace mqttbroker::broker

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace iot::mqtt::server::broker {
    class Broker;
}

#include <core/socket/SocketContext.h>
#include <iot/mqtt/server/SharedSocketContextFactory.h>

//

#include <memory>
#include <nlohmann/json.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

namespace mqttbroker::broker {

    class SharedSocketContextFactory : public iot::mqtt::server::SharedSocketContextFactory<Mqtt> {
    public:
        SharedSocketContextFactory();

        core::socket::SocketContext* create(core::socket::SocketConnection* socketConnection,
                                            std::shared_ptr<iot::mqtt::server::broker::Broker>& broker) final;

    private:
        nlohmann::json jsonMapping;
    };

} // namespace mqttbroker::broker

#endif // APPS_MQTTBROKER_BROKER_SOCKETCONTEXTFACTORY_H
