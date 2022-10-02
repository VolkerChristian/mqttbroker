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

#include "iot/mqtt1/SocketContext.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker1 {

    class SocketContext : public iot::mqtt1::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

        ~SocketContext() override;

    private:
        void initSession();
        void releaseSession();

        void onConnect(const iot::mqtt1::packets::Connect& connect) override;
        void onConnack(const iot::mqtt1::packets::Connack& connack) override;
        void onPublish(const iot::mqtt1::packets::Publish& publish) override;
        void onPuback(const iot::mqtt1::packets::Puback& puback) override;
        void onPubrec(const iot::mqtt1::packets::Pubrec& pubrec) override;
        void onPubrel(const iot::mqtt1::packets::Pubrel& pubrel) override;
        void onPubcomp(const iot::mqtt1::packets::Pubcomp& pubcomp) override;
        void onSubscribe(const iot::mqtt1::packets::Subscribe& subscribe) override;
        void onSuback(const iot::mqtt1::packets::Suback& suback) override;
        void onUnsubscribe(const iot::mqtt1::packets::Unsubscribe& unsubscribe) override;
        void onUnsuback(const iot::mqtt1::packets::Unsuback& unsuback) override;
        void onPingreq(const iot::mqtt1::packets::Pingreq& pingreq) override;
        void onPingresp(const iot::mqtt1::packets::Pingresp& pingresp) override;
        void onDisconnect(const iot::mqtt1::packets::Disconnect& disconnect) override;

        //        uint64_t subscribtionCount = 0;

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

} // namespace mqtt::broker1

#endif // APPS_MQTT_SOCKETCONTEXT_H
