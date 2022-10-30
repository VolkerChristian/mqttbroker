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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <map>
#include <nlohmann/json.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection,
                                 const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                                 const nlohmann::json& jsonMapping)
        : iot::mqtt::server::SocketContext(socketConnection, broker)
        , jsonMapping(jsonMapping) {
    }

    SocketContext::~SocketContext() {
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        LOG(DEBUG) << "Received PUBLISH+++++++++++++++++++++++: " << clientId;
        LOG(DEBUG) << "=================";
        printStandardHeader(publish);
        LOG(DEBUG) << "DUP: " << publish.getDup();
        LOG(DEBUG) << "QoS: " << static_cast<uint16_t>(publish.getQoS());
        LOG(DEBUG) << "Retain: " << publish.getRetain();
        LOG(DEBUG) << "Topic: " << publish.getTopic();
        LOG(DEBUG) << "PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "Message: " << publish.getMessage();

        nlohmann::json subJson = jsonMapping;

        std::string remainingFullTopic = publish.getTopic();
        std::string currentTopic;

        bool currentTopicExistsInMapping = false;

        do {
            std::string::size_type slashPosition = remainingFullTopic.find("/");

            currentTopic = remainingFullTopic.substr(0, slashPosition);
            remainingFullTopic.erase(0, currentTopic.size() + 1);

            currentTopicExistsInMapping = subJson.contains(currentTopic);

            if (!currentTopic.empty() && subJson.is_object() && currentTopicExistsInMapping) {
                subJson = subJson[currentTopic];
            }
        } while (!currentTopic.empty() && subJson.is_object() && currentTopicExistsInMapping);

        if (currentTopic.empty() && subJson.is_object() && subJson.contains("payload")) {
            subJson = subJson["payload"];
            /*
                        enum TopicTypes { STRING, INT, FLOAT, JSON };

                        TopicTypes topicType = TopicTypes::STRING;

                        if (tmpJson.contains("type")) {
                            if (tmpJson["type"] == "string") {
                                topicType = TopicTypes::STRING;
                            } else if (tmpJson["type"] == "int") {
                                topicType = TopicTypes::INT;
                            } else if (tmpJson["type"] == "float") {
                                topicType = TopicTypes::FLOAT;
                            } else if (tmpJson["topic"] == "json") {
                                topicType = TopicTypes::JSON;
                            }
                        }
            */
            if (subJson.contains(publish.getMessage())) {
                subJson = subJson[publish.getMessage()];
                if (subJson.contains("command_topic") && subJson.contains("state")) {
                    const std::string& topic = subJson["command_topic"];
                    const std::string& message = subJson["state"];

                    LOG(INFO) << "Topic mapping found:";
                    LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << topic << ":" << message;

                    this->publish(topic, message, publish.getQoS());
                }
            }
            /*
              else {
                switch (topicType) {
                    case TopicTypes::STRING:
                        break;
                    case TopicTypes::INT:
                        break;
                    case TopicTypes::FLOAT:
                        break;
                    case TopicTypes::JSON:
                        break;
                }

                if (tmpJson.contains("*")) {
                    tmpJson = tmpJson["*"];
                    if (tmpJson.contains("command_topic")) {
                        this->publish(tmpJson["command_topic"], publish.getMessage(), publish.getQoS());
                    }
                }
            }
            */
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
