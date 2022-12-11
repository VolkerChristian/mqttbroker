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

#ifndef WEB_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHO_H
#define WEB_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHO_H

#include "web/websocket/client/SubProtocol.h"

#include <iot/mqtt/MqttContext.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for std::size_t
#include <cstdint> // for uint16_t
#include <string>  // for allocator, string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::subprotocol::echo::client {

    class Echo
        : public web::websocket::client::SubProtocol
        , public iot::mqtt::MqttContext {
    public:
        explicit Echo(const std::string& name);
        ~Echo() override = default;

        std::size_t receive(char* junk, std::size_t junklen) const override;
        void send(char* junk, std::size_t junklen) const override;

        void setKeepAlive(const utils::Timeval& timeout) override;

        void end(bool fatal = false) override;
        void kill() override;

    private:
        void onConnected() override;
        void onMessageStart(int opCode) override;
        void onMessageData(const char* junk, std::size_t junkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;
        void onPongReceived() override;
        void onDisconnected() override;

        std::string data;

        int flyingPings = 0;
    };

} // namespace web::websocket::subprotocol::echo::client

#endif // WEB_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHO_H
