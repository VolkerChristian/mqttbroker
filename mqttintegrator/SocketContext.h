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

#ifndef APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H

#include "lib/MqttMapper.h"

#include <core/timer/Timer.h> // IWYU pragma: export
#include <iot/mqtt/client/SocketContext.h>

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt {
    namespace packets {
        class Publish;
        class Connack;
    } // namespace packets
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::integrator {

    class SocketContext
        : public iot::mqtt::client::SocketContext
        , public apps::mqttbroker::lib::MqttMapper {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection,
                               const nlohmann::json& connectionJson,
                               const nlohmann::json& mappingJson);

        ~SocketContext() override;

    private:
        void onConnected() final;
        void onExit() final;

        void onConnack(iot::mqtt::packets::Connack& connack) final;
        void onPublish(iot::mqtt::packets::Publish& publish) final;

        void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) final;

        const nlohmann::json& connectionJson;

        core::timer::Timer pingTimer;

        uint16_t keepAlive;
        std::string clientId;
        bool cleanSession;
        std::string willTopic;
        std::string willMessage;
        uint8_t willQoS;
        bool willRetain;
        std::string username;
        std::string password;
    };

} // namespace apps::mqttbroker::integrator

#endif // APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXT_H
