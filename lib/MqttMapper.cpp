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

#include "MqttMapper.h"

#include "iot/mqtt/Topic.h"
#include "iot/mqtt/packets/Publish.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "inja.hpp"
#include "log/Logger.h"

#include <initializer_list>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>

// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::lib {

    MqttMapper::MqttMapper(const nlohmann::json& jsonMapping)
        : jsonMapping(jsonMapping) {
    }

    void MqttMapper::extractTopics(const nlohmann::json& json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        for (auto& [key, subJsonMapping] : json.items()) {
            if (subJsonMapping.is_object() && subJsonMapping.contains("payload")) {
                uint8_t qoS = 0;
                if (subJsonMapping.contains("subscribtion")) {
                    nlohmann::json subscribtion = subJsonMapping["subscribtion"];
                    if (subscribtion.is_object() && subscribtion.contains("qos")) {
                        qoS = subscribtion["qos"];
                    }
                }

                topicList.push_back(iot::mqtt::Topic(topic + (topic.empty() || topic == "/" ? "" : "/") + key, qoS));
            }
            if (key != "payload" && subJsonMapping.is_object()) {
                extractTopics(subJsonMapping, topic + (topic.empty() || topic == "/" ? "" : "/") + key, topicList);
            }
        }
    }

    std::list<iot::mqtt::Topic> MqttMapper::extractTopics() {
        std::list<iot::mqtt::Topic> topicList;

        MqttMapper::extractTopics(jsonMapping, "", topicList);

        return topicList;
    }

    void MqttMapper::publishTemplate(const nlohmann::json& subJsonMapping,
                                     const nlohmann::json& json,
                                     const iot::mqtt::packets::Publish& publish) {
        if (subJsonMapping.contains("command_topic") && subJsonMapping.contains("state_template")) {
            const std::string& topic = subJsonMapping["command_topic"];
            const std::string& stateTemplate = subJsonMapping["state_template"];

            try {
                // Render
                std::string renderedMessage = inja::render(stateTemplate, json);

                LOG(INFO) << "      " << stateTemplate << " -> " << renderedMessage;

                publishMapping(topic, renderedMessage, publish.getQoS());
            } catch (const inja::InjaError& e) {
                LOG(ERROR) << e.what();
            }
        }
    }

    void MqttMapper::publishTemplates(const nlohmann::json& subJsonMapping,
                                      const nlohmann::json& json,
                                      const iot::mqtt::packets::Publish& publish) {
        LOG(INFO) << "Topic mapping found:";
        LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << json.dump();

        if (subJsonMapping.is_object()) {
            publishTemplate(subJsonMapping, json, publish);
        } else if (subJsonMapping.is_array()) {
            for (const nlohmann::json& elementJson : subJsonMapping) {
                publishTemplate(elementJson, json, publish);
            }
        }
    }

    void MqttMapper::publishMappings(const iot::mqtt::packets::Publish& publish) {
        nlohmann::json subJsonMapping = jsonMapping;

        std::string remainingTopic = publish.getTopic();
        std::string topicLevel;

        bool currentTopicExistsInMapping = false;

        do {
            std::string::size_type slashPosition = remainingTopic.find("/");

            topicLevel = remainingTopic.substr(0, slashPosition);
            remainingTopic.erase(0, topicLevel.size() + 1);

            currentTopicExistsInMapping = subJsonMapping.contains(topicLevel);
            if (!topicLevel.empty() && currentTopicExistsInMapping) {
                subJsonMapping = subJsonMapping[topicLevel];
            }
        } while (!topicLevel.empty() && currentTopicExistsInMapping);

        if (topicLevel.empty() && subJsonMapping.contains("payload")) {
            subJsonMapping = subJsonMapping["payload"];

            if (subJsonMapping.contains("static")) {
                subJsonMapping = subJsonMapping["static"];

                if (subJsonMapping.contains("command_topic") && subJsonMapping.contains(publish.getMessage())) {
                    const std::string& topic = subJsonMapping["command_topic"];
                    subJsonMapping = subJsonMapping[publish.getMessage()];

                    if (subJsonMapping.contains("state")) {
                        const std::string& message = subJsonMapping["state"];

                        LOG(INFO) << "Topic mapping found:";
                        LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << topic << ":" << message;

                        publishMapping(topic, message, publish.getQoS());
                    }
                }
            } else {
                try {
                    nlohmann::json json;

                    if (subJsonMapping.contains("value")) {
                        subJsonMapping = subJsonMapping["value"];

                        json["value"] = publish.getMessage();
                    } else if (subJsonMapping.contains("json")) {
                        subJsonMapping = subJsonMapping["json"];

                        json["json"] = nlohmann::json::parse(publish.getMessage());
                    }

                    publishTemplates(subJsonMapping, json, publish);
                } catch (const nlohmann::json::exception& e) {
                    LOG(ERROR) << e.what();
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
        "connection" : {
            "keep_alive" : 60,
            "client_id" : "Client",
            "clean_session" : true,
            "will_topic" : "will/topic",
            "will_message" : "Last Will",
            "will_qos": 0,
            "will_retain" : false,
            "username" : "Username",
            "password" : "Password"
        },
        "mapping" : {
            "iotempower" : {
                "test01" : {
                    "button1" : {
                        "type" : "binary_sensor",
                        "subscribtion" : {
                            "qos" : 2
                        },
                        "payload": {
                            "static" : {
                                "command_topic" : "test02/onboard/set",
                                "pressed" : {
                                    "state" : "on"
                                },
                                "released": {
                                    "state" : "off"
                                }
                            },
                            "value" : {
                                "command_topic" : "test02/onboard/set",
                                "state_template" : "{{ value }}pm"
                            },
                            "json" : {
                                "command_topic" : "test02/onboard/set",
                                "state_template" : "{{ json.time.start }} to {{ json.time.end + 1 }}pm"
                            }
                        }
                    }
                }
            }
        }
    }

    */

} // namespace apps::mqttbroker::lib
