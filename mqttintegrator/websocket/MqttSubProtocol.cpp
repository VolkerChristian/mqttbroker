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

#include "MqttSubProtocol.h"

#include "mqttintegrator/lib/Mqtt.h"

#include <log/Logger.h>
#include <web/websocket/SubProtocolContext.h>

//

#include <algorithm>
#include <iomanip>
#include <ostream>

#define PING_INTERVAL 0
#define MAX_FLYING_PINGS 3

namespace mqtt::mqttintegrator::websocket {

    MqttSubProtocol::MqttSubProtocol(web::websocket::SubProtocolContext* subProtocolContext,
                                     const std::string& name,
                                     const nlohmann::json& connectionJson,
                                     const nlohmann::json& mappingJson)
        : web::websocket::client::SubProtocol(subProtocolContext, name, PING_INTERVAL, MAX_FLYING_PINGS)
        , iot::mqtt::MqttContext(new mqtt::mqttintegrator::lib::Mqtt(connectionJson, mappingJson))
        , onReceivedFromPeerEvent([this]() -> void {
            iot::mqtt::MqttContext::onReceiveFromPeer();
        }) {
    }

    MqttSubProtocol::~MqttSubProtocol() {
        keepAliveTimer.cancel();
    }

    std::size_t MqttSubProtocol::receive(char* junk, std::size_t junklen) {
        std::size_t maxReturn = std::min(junklen, size);

        std::copy(buffer.data() + cursor, buffer.data() + cursor + maxReturn, junk);

        cursor += maxReturn;
        size -= maxReturn;

        if (size > 0) {
            onReceivedFromPeerEvent.publish();
        } else {
            buffer.clear();
            cursor = 0;
            size = 0;
        }

        return maxReturn;
    }

    void MqttSubProtocol::send(const char* junk, std::size_t junklen) {
        sendMessage(junk, junklen);
    }

    void MqttSubProtocol::setKeepAlive(const utils::Timeval& timeout) {
        keepAliveTimer = core::timer::Timer::singleshotTimer(
            [this, timeout]() -> void {
                LOG(TRACE) << "Keep-alive timer expired. Interval was: " << timeout;
                sendClose();
            },
            timeout);

        getSocketConnection()->setTimeout(0);
    }

    void MqttSubProtocol::end([[maybe_unused]] bool fatal) {
        sendClose();
    }

    void MqttSubProtocol::kill() {
        sendClose();
    }

    void MqttSubProtocol::onConnected() {
        VLOG(0) << "Mqtt connected:";
        iot::mqtt::MqttContext::onConnected();
    }

    void MqttSubProtocol::onMessageStart(int opCode) {
        if (opCode == 1) {
            this->end(true);
        }
    }

    void MqttSubProtocol::onMessageData(const char* junk, std::size_t junkLen) {
        data += std::string(junk, junkLen);
    }

    void MqttSubProtocol::onMessageEnd() {
        std::stringstream ss;

        ss << "WS-Message: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i != data.size()) {
                ss << std::endl;
                ss << "                                            ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
               << " "; // << " | ";
        }
        VLOG(0) << ss.str();

        buffer.insert(buffer.end(), data.begin(), data.end());
        size += data.size();
        data.clear();

        if (buffer.size() > 0) {
            iot::mqtt::MqttContext::onReceiveFromPeer();
        }

        keepAliveTimer.restart();
    }

    void MqttSubProtocol::onMessageError(uint16_t errnum) {
        VLOG(0) << "Message error: " << errnum;
    }

    void MqttSubProtocol::onDisconnected() {
        VLOG(0) << "MQTT disconnected:";
        iot::mqtt::MqttContext::onDisconnected();
    }

    void MqttSubProtocol::onExit() {
        VLOG(0) << "MQTT exit:";
        iot::mqtt::MqttContext::onExit();
    }

    core::socket::SocketConnection* MqttSubProtocol::getSocketConnection() {
        return getSubProtocolContext()->getSocketConnection();
    }

} // namespace mqtt::mqttintegrator::websocket
