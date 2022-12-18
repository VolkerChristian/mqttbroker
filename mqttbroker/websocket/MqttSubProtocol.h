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

#ifndef WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOL_H
#define WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOL_H

namespace iot::mqtt::server::broker {
    class Broker;
}

namespace web::websocket {
    class SubProtocolContext;
}

#include <core/EventReceiver.h>
#include <core/socket/SocketConnection.h>
#include <iot/mqtt/MqttContext.h>
#include <utils/Timeval.h>
#include <web/websocket/server/SubProtocol.h>

//

#include <core/timer/Timer.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace mqttbroker::broker::websocket {

    class OnReceivedFromPeerEvent : public core::EventReceiver {
    public:
        explicit OnReceivedFromPeerEvent(const std::function<void(void)>& event)
            : core::EventReceiver("WS-OnData")
            , event(event) {
        }

    private:
        void onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
            event();
        }

        std::function<void(void)> event;
    };

    class MqttSubProtocol
        : public web::websocket::server::SubProtocol
        , public iot::mqtt::MqttContext {
    public:
        MqttSubProtocol(web::websocket::SubProtocolContext* subProtocolContext,
                        const std::string& name,
                        const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                        const nlohmann::json& mappingJson);
        ~MqttSubProtocol() override;

        std::size_t receive(char* junk, std::size_t junklen) override;
        void send(const char* junk, std::size_t junklen) override;

        void setKeepAlive(const utils::Timeval& timeout) override;

        void end(bool fatal = false) override;
        void kill() override;

    private:
        void onConnected() override;
        void onMessageStart(int opCode) override;
        void onMessageData(const char* junk, std::size_t junkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;
        void onDisconnected() override;
        void onExit() override;

        core::socket::SocketConnection* getSocketConnection() override;

        OnReceivedFromPeerEvent onReceivedFromPeerEvent;

        core::timer::Timer keepAliveTimer;

        std::string data;
        std::vector<char> buffer;
        std::size_t cursor = 0;
        std::size_t size = 0;
    };

} // namespace mqttbroker::broker::websocket

#endif // WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOL_H
