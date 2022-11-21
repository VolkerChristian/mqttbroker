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

#ifndef APPS_MQTTBROKER_MQTTWETFRONTEND_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_MQTTWETFRONTEND_SOCKETCONTEXT_H

#include "iot/mqtt/server/SocketContext.h"
#include "lib/MqttMapper.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt {
    namespace packets {
        class Publish;
    }
    namespace server::broker {
        class Broker;
    }
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::webfrontend {

    class SocketContext
        : public iot::mqtt::server::SocketContext
        , public apps::mqttbroker::lib::MqttMapper {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection,
                               const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                               const nlohmann::json& mappingJson);

    private:
        // inherited from iot::mqtt::server::SocketContext - the plain and base MQTT broker
        void onConnect(iot::mqtt::packets::Connect& connect) override;
        void onPublish(iot::mqtt::packets::Publish& publish) override;

        // inherited from core::socket::SocketContext, the root class of all SocketContext classes
        void onDisconnected() override;

        // inherited from apps::mqttbroker::lib::MqttMapper
        void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) override;
    };

} // namespace apps::mqttbroker::webfrontend

#endif // APPS_MQTTBROKER_MQTTWETFRONTEND_SOCKETCONTEXT_H
