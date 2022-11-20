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

    MqttMapper::MqttMapper(const nlohmann::json& mappingJson)
        : mappingJson(mappingJson) {
    }

    std::string MqttMapper::dump() {
        return mappingJson.dump();
    }

    void MqttMapper::extractTopics(const nlohmann::json& json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        for (auto& [item, mappingSubJson] : json.items()) {
            if (item == "topic_level" && mappingSubJson.contains("name") && mappingSubJson["name"].is_string()) {
                std::string subTopic = mappingSubJson["name"];

                if (mappingSubJson.is_object() && mappingSubJson.contains("mapping")) {
                    uint8_t qoS = 0;
                    if (mappingSubJson.contains("subscribtion")) {
                        nlohmann::json subscribtion = mappingSubJson["subscribtion"];
                        if (subscribtion.is_object() && subscribtion.contains("qos")) {
                            qoS = subscribtion["qos"];
                        }
                    }

                    topicList.push_back(
                        iot::mqtt::Topic(topic + ((topic.empty() || topic == "/") && !subTopic.empty() ? "" : "/") + subTopic, qoS));
                }
                if (mappingSubJson.is_object() && mappingSubJson.contains("topic_level")) {
                    extractTopics(
                        mappingSubJson, topic + ((topic.empty() || topic == "/") && !subTopic.empty() ? "" : "/") + subTopic, topicList);
                }
            }
        }
    }

    std::list<iot::mqtt::Topic> MqttMapper::extractTopics() {
        std::list<iot::mqtt::Topic> topicList;

        MqttMapper::extractTopics(mappingJson, "", topicList);

        return topicList;
    }

    void MqttMapper::publishTemplate(const nlohmann::json& mappingSubJson,
                                     const nlohmann::json& json,
                                     const iot::mqtt::packets::Publish& publish) {
        if (mappingSubJson.contains("command_topic") && mappingSubJson.contains("mapping_template")) {
            const std::string& topic = mappingSubJson["command_topic"];
            const std::string& stateTemplate = mappingSubJson["mapping_template"];
            bool retain = mappingSubJson.value("retain_message", false);

            try {
                // Render
                std::string renderedMessage = inja::render(stateTemplate, json);

                LOG(INFO) << "      " << topic << " : " << stateTemplate << " -> '" << renderedMessage << "'";

                publishMapping(topic, renderedMessage, publish.getQoS(), retain);
            } catch (const inja::InjaError& e) {
                LOG(ERROR) << e.what();
            }
        }
    }

    void MqttMapper::publishTemplates(const nlohmann::json& mappingSubJson,
                                      const nlohmann::json& json,
                                      const iot::mqtt::packets::Publish& publish) {
        LOG(INFO) << "  " << publish.getTopic() << " : " << publish.getMessage() << " -> " << json.dump();

        if (mappingSubJson.is_object()) {
            publishTemplate(mappingSubJson, json, publish);
        } else if (mappingSubJson.is_array()) {
            for (const nlohmann::json& elementJson : mappingSubJson) {
                publishTemplate(elementJson, json, publish);
            }
        }
    }

    void MqttMapper::publishMappings(const iot::mqtt::packets::Publish& publish) {
        nlohmann::json mappingSubJson = mappingJson;

        std::string remainingTopic = publish.getTopic();
        std::string topicLevelName;

        bool currentTopicExistsInMapping = false;

        do {
            std::string::size_type slashPosition = remainingTopic.find("/");

            topicLevelName = remainingTopic.substr(0, slashPosition);
            remainingTopic.erase(0, topicLevelName.size() + 1);

            currentTopicExistsInMapping = mappingSubJson.contains("topic_level") && mappingSubJson["topic_level"].is_object() &&
                                          mappingSubJson["topic_level"].contains("name") &&
                                          mappingSubJson["topic_level"]["name"] == topicLevelName;
            if ((!topicLevelName.empty() || !remainingTopic.empty()) && currentTopicExistsInMapping) {
                mappingSubJson = mappingSubJson["topic_level"];
            }
        } while ((!topicLevelName.empty() || !remainingTopic.empty()) && currentTopicExistsInMapping);

        if (topicLevelName.empty() && mappingSubJson.contains("mapping")) {
            mappingSubJson = mappingSubJson["mapping"];

            if (mappingSubJson.contains("static")) {
                mappingSubJson = mappingSubJson["static"];

                if (mappingSubJson.contains("command_topic") && mappingSubJson.contains("message_mappings") &&
                    mappingSubJson["message_mappings"].is_array()) {
                    const std::string& commandTopic = mappingSubJson["command_topic"];
                    bool retain = mappingSubJson.value("retain_message", false);
                    mappingSubJson = mappingSubJson["message_mappings"];

                    const nlohmann::json& concreteMapping =
                        *std::find_if(mappingSubJson.begin(), mappingSubJson.end(), [&publish](const nlohmann::json& mapping) {
                            return mapping.is_object() && mapping.value("message", "") == publish.getMessage();
                        });

                    if (concreteMapping.is_object() && concreteMapping.contains("mapped_message")) {
                        const std::string& message = concreteMapping["mapped_message"];

                        LOG(INFO) << "Topic mapping found:";
                        LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << commandTopic << ":" << message;

                        publishMapping(commandTopic, message, publish.getQoS(), retain);
                    }
                }
            } else {
                try {
                    nlohmann::json json;

                    if (mappingSubJson.contains("value")) {
                        mappingSubJson = mappingSubJson["value"];

                        json["value"] = publish.getMessage();
                    } else if (mappingSubJson.contains("json")) {
                        mappingSubJson = mappingSubJson["json"];

                        json["json"] = nlohmann::json::parse(publish.getMessage());
                    }

                    if (!json.empty()) {
                        LOG(INFO) << "Topic mapping found:";
                    }

                    publishTemplates(mappingSubJson, json, publish);
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
        "mappings" : {
            "discover_prefix" : "iotempower",
            "topic_level" : {
                "name" : "test01",
                "topic_level" : {
                    "name" : "button1",
                    "type" : "binary_sensor",
                    "subscribtion" : {
                        "qos" : 2
                    },
                    "mapping": {
                        "Comment" : "Either a \"static\", \"value\", or \"json\" object is required! E.g. next three objects. Exclusive
    order of interpretation: \"static\", \"value\", \"json\"", "static1" : { "command_topic" : "test02/onboard/set", "message_mappings" : [
                                {
                                    "message" : "pressed",
                                    "mapped_message" : "on"
                                },
                                {
                                    "message" : "released",
                                    "mapped_message" : "off"
                                }
                            ]
                        },
                        "value" : {
                            "command_topic" : "test02/onboard/set",
                            "mapping_template" : "{% if value == \"pressed\" %}on{% else if value == \"released\" %}off{% endif %}"
                        },
                        "json" : {
                            "command_topic" : "test02/onboard/set",
                            "mapping_template" : "{{ json.time.start }} to {{ json.time.end + 1 }}pm"
                        },
                        "The next objects are ignored" : "Listed only for documentation",
                        "value | Comment: An array of \"value\" objects is also allowed for multi-mapping. E.g.:" : [
                            {
                                "command_topic" : "aCommandTopic",
                                "mapping_template" : "aStateTemplate"
                            },
                            {
                                "command_topic" : "anOtherCommandTopic",
                                "mapping_template" : "anOtherStateTemplate"
                            }

                        ],
                        "json | Comment: An array of \"json\" objects is also allowed for multi-mapping. E.g.:" : [
                            {
                                "command_topic" : "aCommandTopic",
                                "mapping_template" : "aStateTemplate"
                            },
                            {
                                "command_topic" : "anOtherCommandTopic",
                                "mapping_template" : "anOtherStateThemplate"
                            }
                        ]
                    },
                    "topic_level" : {
                        "name" : "someothertopiclevelname",
                        "type" : "binary_sensor",
                        "subscribtion" : {
                            "qos" : 1
                        },
                        "mapping" : {
                            "static" : {
                                "command_topic" : "some/other/topic/set",
                                "message_mappings" : [
                                    {
                                        "message" : "pressed",
                                        "mapped_message" : "on"
                                    },
                                    {
                                        "message" : "released",
                                        "mapped_message" : "off"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        }
    }

    */

} // namespace apps::mqttbroker::lib
