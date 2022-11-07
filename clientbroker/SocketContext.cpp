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
#include "iot/mqtt/packets/Publish.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <map>
#include <nlohmann/json.hpp>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection,
                                 const nlohmann::json& connection,
                                 const nlohmann::json& jsonMapping)
        : iot::mqtt::client::SocketContext(socketConnection)
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

    void SocketContext::extractTopics(nlohmann::json json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        for (auto& [key, value] : json.items()) {
            if (value.is_object(), value.contains("payload")) {
                uint8_t qoS = 0;
                if (value.contains("subscribtion")) {
                    nlohmann::json subscribtion = value["subscribtion"];
                    if (subscribtion.is_object() && subscribtion.contains("qos")) {
                        qoS = subscribtion["qos"];
                    }
                }

                topicList.push_back(iot::mqtt::Topic(topic + (topic.empty() || topic == "/" ? "" : "/") + key, qoS));
            }
            if (key != "payload" && key != "qos" && value.is_object()) {
                extractTopics(value, topic + (topic.empty() || topic == "/" ? "" : "/") + key, topicList);
            }
        }
    }

    std::list<iot::mqtt::Topic> SocketContext::extractTopics(nlohmann::json json, const std::string& topic) {
        std::list<iot::mqtt::Topic> topicList;

        extractTopics(json, topic, topicList);

        return topicList;
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

                VLOG(0) << retainedConnect.dump();

                this->sendPublish(++packetIdentifier, "snode.c/_cfg_/connection", retainedConnect.dump(), 0, true);
                this->sendPublish(++packetIdentifier, "snode.c/_cfg_/mapping", jsonMapping.dump(), 0, true);

                std::list<iot::mqtt::Topic> topicList = SocketContext::extractTopics(jsonMapping, "");

                for (const iot::mqtt::Topic& topic : topicList) {
                    LOG(INFO) << "Subscribe Topic: " << topic.getName() << ", qoS: " << static_cast<uint16_t>(topic.getQoS());
                }

                this->sendSubscribe(++packetIdentifier, topicList);
            }
        }
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        nlohmann::json subJson = jsonMapping;

        std::string remainingTopic = publish.getTopic();
        std::string topicLevel;

        bool currentTopicExistsInMapping = false;

        do {
            std::string::size_type slashPosition = remainingTopic.find("/");

            topicLevel = remainingTopic.substr(0, slashPosition);
            remainingTopic.erase(0, topicLevel.size() + 1);

            currentTopicExistsInMapping = subJson.contains(topicLevel);

            if (!topicLevel.empty() && currentTopicExistsInMapping) {
                subJson = subJson[topicLevel];
            }
        } while (!topicLevel.empty() && currentTopicExistsInMapping);

        if (topicLevel.empty() && subJson.contains("payload")) {
            subJson = subJson["payload"];

            if (subJson.contains(publish.getMessage())) {
                subJson = subJson[publish.getMessage()];
                if (subJson.contains("command_topic") && subJson.contains("state")) {
                    const std::string& topic = subJson["command_topic"];
                    const std::string& message = subJson["state"];

                    LOG(INFO) << "Topic mapping found:";
                    LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << topic << ":" << message;

                    sendPublish(++packetIdentifier, topic, message, publish.getQoS());
                }
            }
        }
    }

    /*
        iotempower/cfg/test01/ip 192.168.12.183
        iotempower/cfg/test02/ip 192.168.12.132
        iotempower/binary_sensor/test01/button1/config
        {
            "name": "test01 button1",
            "state_topic": "test01/button1",
            "payload_on": "released",
            "payload_off": "pressed"
        },
        iotempower/switch/test02/onboard/config
        {
            "name": "test02 onboard",
            "state_topic": "test02/onboard",
            "state_on": "on",
            "state_off": "off",
            "command_topic": "test02/onboard/set",
            "payload_on": "on",
            "payload_off": "off"
        }

    {
       "binary_sensor" : [{
            "name": "test01 button1",
            "state_topic": "test01/button1",
            "payload_on": "released",
            "payload_off": "pressed"
        }],
        "switch" : [{
            "name": "test02 onboard",
            "state_topic": "test02/onboard",
            "state_on": "on",
            "state_off": "off",
            "command_topic": "test02/onboard/set",
            "payload_on": "on",
            "payload_off": "off"
        }]
    }

    {
        "iotempower" : {
            "test01" : {
                "button1" : {
                    "qos" : 0,
                    "payload" : {
                        "type" : "binary_sensor",
                        "pressed" : {
                            "command_topic" : "test02/onboard/set",
                            "state" : "on"
                        },
                        "released" : {
                            "command_topic" : "test02/onboard/set",
                            "state" : "off"
                        }
                    }
                }
            }
        }
    }

    */

} // namespace apps::mqttbroker
