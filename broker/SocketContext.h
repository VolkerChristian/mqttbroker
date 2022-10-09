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

#ifndef APPS_MQTT_SERVER_SOCKETCONTEXT_H
#define APPS_MQTT_SERVER_SOCKETCONTEXT_H

#include "iot/mqtt/server/SocketContext.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace mqtt::broker {
    class Broker;
} // namespace mqtt::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    class SocketContext : public iot::mqtt::server::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection, std::shared_ptr<mqtt::broker::Broker> broker);

        ~SocketContext() override;

    private:
        void initSession();
        void releaseSession();

        void onConnect(iot::mqtt::server::packets::Connect& connect) override;
        void onPublish(iot::mqtt::packets::Publish& publish) override;
        void onPuback(iot::mqtt::packets::Puback& puback) override;
        void onPubrec(iot::mqtt::packets::Pubrec& pubrec) override;
        void onPubrel(iot::mqtt::packets::Pubrel& pubrel) override;
        void onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) override;
        void onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe) override;
        void onUnsubscribe(iot::mqtt::server::packets::Unsubscribe& unsubscribe) override;
        void onPingreq(iot::mqtt::server::packets::Pingreq& pingreq) override;
        void onDisconnect(iot::mqtt::server::packets::Disconnect& disconnect) override;

        void printStandardHeader(const iot::mqtt::ControlPacketReceiver& packet);

        std::shared_ptr<mqtt::broker::Broker> broker;

        // V-Header
        std::string protocol;
        uint8_t level = 0;
        uint8_t connectFlags = 0;
        uint16_t keepAlive = 0;

        // Payload
        std::string clientId;
        std::string willTopic;
        std::string willMessage;
        std::string username;
        std::string password;

        // Derived from flags
        bool usernameFlag = false;
        bool passwordFlag = false;
        bool willRetain = false;
        uint8_t willQoS = 0;
        bool willFlag = false;
        bool cleanSession = false;
    };

} // namespace mqtt::broker

#endif // APPS_MQTT_SOCKETCONTEXT_H
