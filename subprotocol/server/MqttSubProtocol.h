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

#include <core/EventReceiver.h>
#include <web/websocket/server/SubProtocol.h>

namespace iot::mqtt::server::broker {
    class Broker;
}
namespace web::websocket {
    class SubProtocolContext;
}

namespace core::socket {
    class SocketConnection;
}

#include <core/timer/Timer.h>
#include <iot/mqtt/MqttContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // for uint16_t
#include <functional>
#include <memory> // for shared_ptr
#include <nlohmann/json_fwd.hpp>
#include <string>          // for allocator, string
#include <utils/Timeval.h> // for size_t, Timeval
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::subprotocol::echo::server {

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
        MqttSubProtocol(SubProtocolContext* subProtocolContext,
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

} // namespace web::websocket::subprotocol::echo::server

#endif // WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOL_H
