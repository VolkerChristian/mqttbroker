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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection,
                                 const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker)
        : iot::mqtt::server::SocketContext(socketConnection, broker) {
    }

    SocketContext::~SocketContext() {
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        LOG(DEBUG) << "Received PUBLISH: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(publish);
        LOG(DEBUG) << "DUP: " << publish.getDup();
        LOG(DEBUG) << "QoS: " << static_cast<uint16_t>(publish.getQoS());
        LOG(DEBUG) << "Retain: " << publish.getRetain();
        LOG(DEBUG) << "Topic: " << publish.getTopic();
        LOG(DEBUG) << "PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "Message: " << publish.getMessage();

        if (publish.getTopic() == "test01/button1") {
            VLOG(0) << "Found correct topic";
            if (publish.getMessage() == "pressed") {
                VLOG(0) << "Found message 'pressed'";
                this->publish("test02/onboard/set", "on", publish.getQoS());
            } else if (publish.getMessage() == "released") {
                VLOG(0) << "Found message 'released'";
                this->publish("test02/onboard/set", "off", publish.getQoS());
            }
        }
    }

} // namespace apps::mqttbroker
