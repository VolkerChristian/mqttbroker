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

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection, const nlohmann::json& jsonMapping)
        : iot::mqtt::client::SocketContext(socketConnection)
        , jsonMapping(jsonMapping) {
    }

    SocketContext::~SocketContext() {
        pingTimer.cancel();
    }

    void SocketContext::onConnected() {
        VLOG(0) << "On Connected";
        uint16_t keepAlive = 60;

        this->sendConnect(keepAlive, "voc", true, "", "", 0, false, "", "");

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
            if (value.contains("payload")) {
                uint8_t qoS = 0;
                if (value.contains("qos")) {
                    qoS = value["qos"];
                }

                topicList.push_back(iot::mqtt::Topic(topic + (topic.empty() || topic == "/" ? "" : "/") + key, qoS));
            }
            if (key != "payload" && key != "qos" && value.is_object()) {
                extractTopics(value, topic + (topic.empty() || topic == "/" ? "" : "/") + key, topicList);
            }
        }
    }

    void SocketContext::onConnack([[maybe_unused]] iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0) {
            if (!connack.getSessionPresent()) {
                std::list<iot::mqtt::Topic> topicList;

                SocketContext::extractTopics(jsonMapping, "", topicList);

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

                    this->sendPublish(++packetIdentifier, topic, message, publish.getQoS());
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
                    "payload" : {
                        "type" : "string",
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
