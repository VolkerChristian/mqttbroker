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

#include "SocketContext.h"

#include "MqttModel.h"

#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/server/broker/Broker.h>
#include <iot/mqtt/server/broker/Message.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::webfrontend {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection,
                                 const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                                 const nlohmann::json& mappingJson)
        : iot::mqtt::server::SocketContext(socketConnection, broker)
        , apps::mqttbroker::lib::MqttMapper(mappingJson) {
    }

    void SocketContext::onConnect(iot::mqtt::packets::Connect& connect) {
        MqttModel::instance().addConnectedClient(this, connect);
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        publishMappings(publish);
    }

    void SocketContext::publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        broker->publish(topic, message, qoS);

        if (retain) {
            broker->retainMessage(topic, message, qoS);
        }

        publishMappings(iot::mqtt::packets::Publish(getPacketIdentifier(), topic, message, qoS, retain, MQTT_DUP_FALSE));
    }

    void SocketContext::onDisconnected() {
        MqttModel::instance().delDisconnectedClient(this);
    }

} // namespace apps::mqttbroker::webfrontend
