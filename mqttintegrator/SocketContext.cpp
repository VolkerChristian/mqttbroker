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

#include "SocketContext.h" // IWYU pragma: export

#include "iot/mqtt/packets/Connack.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <list>
#include <map>
#include <nlohmann/json.hpp>

// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::integrator {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection,
                                 const nlohmann::json& connection,
                                 const nlohmann::json& jsonMapping)
        : iot::mqtt::client::SocketContext(socketConnection)
        , apps::mqttbroker::lib::MqttMapper(jsonMapping)
        , connection(connection)
        , jsonMapping(jsonMapping) {
        keepAlive = connection.contains("keep_alive") ? static_cast<uint16_t>(connection["keep_alive"]) : 60;
        clientId = connection.contains("client_id") ? static_cast<std::string>(connection["client_id"]) : "";
        cleanSession = connection.contains("clean_session") ? static_cast<bool>(connection["clean_session"]) : true;
        willTopic = connection.contains("will_topic") ? static_cast<std::string>(connection["will_topic"]) : "";
        willMessage = connection.contains("will_message") ? static_cast<std::string>(connection["will_message"]) : "";
        willQoS = connection.contains("will_qos") ? static_cast<uint8_t>(connection["will_qos"]) : 60;
        willRetain = connection.contains("will_retain") ? static_cast<bool>(connection["will_retain"]) : true;
        username = connection.contains("username") ? static_cast<std::string>(connection["username"]) : "";
        password = connection.contains("password") ? static_cast<std::string>(connection["password"]) : "";

        LOG(TRACE) << "Keep Alive: " << keepAlive;
        LOG(TRACE) << "Client Id: " << clientId;
        LOG(TRACE) << "Clean Session: " << cleanSession;
        LOG(TRACE) << "Will Topic: " << willTopic;
        LOG(TRACE) << "Will Message: " << willMessage;
        LOG(TRACE) << "Will QoS: " << static_cast<uint16_t>(willQoS);
        LOG(TRACE) << "Will Retain " << willRetain;
        LOG(TRACE) << "Username: " << username;
        LOG(TRACE) << "Password: " << password;
    }

    SocketContext::~SocketContext() {
        pingTimer.cancel();
    }

    void SocketContext::onConnected() {
        VLOG(0) << "On Connected";

        if (connection.contains("keep_alive"))

            this->sendConnect(keepAlive, clientId, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);

        this->setTimeout(keepAlive * 1.5);

        pingTimer = core::timer::Timer::intervalTimer(
            [this](void) -> void {
                this->sendPingreq();
            },
            keepAlive);
    }

    void SocketContext::onExit() {
        VLOG(0) << "On Exit";
        this->sendDisconnect();
    }

    void SocketContext::onDisconnected() {
        VLOG(0) << "On Disconnected";
    }

    void SocketContext::onConnack(iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0) {
            if (!connack.getSessionPresent()) {
                nlohmann::json retainedConnect;
                retainedConnect["keep_alive"] = keepAlive;
                retainedConnect["client_id"] = clientId;
                retainedConnect["clean_session"] = cleanSession;
                retainedConnect["will_topic"] = willTopic;
                retainedConnect["will_message"] = willMessage;
                retainedConnect["will_qos"] = willQoS;
                retainedConnect["will_retain"] = willRetain;
                retainedConnect["username"] = username;
                retainedConnect["password"] = password;

                this->sendPublish(++packetIdentifier, "snode.c/_cfg_/connection", retainedConnect.dump(), 0, true);
                this->sendPublish(++packetIdentifier, "snode.c/_cfg_/mapping", jsonMapping.dump(), 0, true);

                std::list<iot::mqtt::Topic> topicList = MqttMapper::extractTopics();

                for (const iot::mqtt::Topic& topic : topicList) {
                    LOG(INFO) << "Subscribe Topic: " << topic.getName() << ", qoS: " << static_cast<uint16_t>(topic.getQoS());
                }

                this->sendSubscribe(++packetIdentifier, topicList);
            }
        }
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        publishMappings(publish);
    }

    void SocketContext::publishMappingMatch(const std::string& topic, const std::string& message, uint8_t qoS) {
        sendPublish(++packetIdentifier, topic, message, qoS);
    }

} // namespace apps::mqttbroker::integrator
