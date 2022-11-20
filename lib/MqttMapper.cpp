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

    void MqttMapper::extractTopic(const nlohmann::json& topicLevel, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        if (topicLevel.is_object() && topicLevel.contains("name")) {
            const nlohmann::json& nameJson = topicLevel.value("name", "");

            if (nameJson.is_string()) {
                std::string name = nameJson.get<std::string>();

                const nlohmann::json& topicLevelMapping = topicLevel.value("mapping", nlohmann::json::object());

                if (topicLevelMapping.is_object() && !topicLevelMapping.empty()) {
                    const nlohmann::json& subscribtionJson = topicLevel.value("subscribtion", nlohmann::json::object());
                    uint8_t qoS = 0;

                    if (subscribtionJson.is_object()) {
                        qoS = subscribtionJson.value<uint8_t>("qos", 0);
                    }

                    topicList.push_back(
                        iot::mqtt::Topic(topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, qoS));
                }

                if (topicLevel.contains("topic_level")) {
                    extractTopics(topicLevel, topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, topicList);
                }
            }
        }
    }

    void MqttMapper::extractTopics(const nlohmann::json& json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        if (json.is_object()) {
            const nlohmann::json& topicLevels = json.value("topic_level", nlohmann::json::array());

            if (topicLevels.is_array() && !topicLevels.empty()) {
                for (const nlohmann::json& topicLevel : topicLevels) {
                    extractTopic(topicLevel, topic, topicList);
                }
            } else {
                extractTopic(topicLevels, topic, topicList);
            }
        }
    }

    std::list<iot::mqtt::Topic> MqttMapper::extractTopics() {
        std::list<iot::mqtt::Topic> topicList;

        try {
            MqttMapper::extractTopics(mappingJson, "", topicList);
        } catch (const nlohmann::json::exception& e) {
            LOG(ERROR) << e.what();
        }

        return topicList;
    }

    void MqttMapper::publishTemplate(const nlohmann::json& templateMapping,
                                     const nlohmann::json& json,
                                     const iot::mqtt::packets::Publish& publish) {
        const std::string& commandTopic = templateMapping.value("command_topic", "");
        const std::string& mappingTemplate = templateMapping.value("mapping_template", "");
        bool retain = templateMapping.value("retain_message", false);

        if (commandTopic.empty()) {
            LOG(WARNING) << "No \"command_topic\" in mapping '" << templateMapping.dump() << "'";
        } else if (mappingTemplate.empty()) {
            LOG(WARNING) << "No \"mapping_template\" in mapping '" << templateMapping.dump() << "'";
        } else {
            try {
                // Render
                std::string renderedMessage = inja::render(mappingTemplate, json);

                LOG(INFO) << "      " << commandTopic << " : " << mappingTemplate << " -> '" << renderedMessage << "'";

                publishMapping(commandTopic, renderedMessage, publish.getQoS(), retain);
            } catch (const inja::InjaError& e) {
                LOG(ERROR) << e.what();
            }
        }
    }

    void MqttMapper::publishTemplates(const nlohmann::json& templateMapping,
                                      const nlohmann::json& json,
                                      const iot::mqtt::packets::Publish& publish) {
        LOG(INFO) << "  " << publish.getTopic() << " : " << publish.getMessage() << " -> " << json.dump();

        if (templateMapping.is_object()) {
            publishTemplate(templateMapping, json, publish);
        } else if (templateMapping.is_array()) {
            for (const nlohmann::json& concreteTemplateMapping : templateMapping) {
                publishTemplate(concreteTemplateMapping, json, publish);
            }
        }
    }

    void MqttMapper::publishMappings(const iot::mqtt::packets::Publish& publish) {
        std::string remainingTopic = publish.getTopic();

        if (!mappingJson.empty()) {
            nlohmann::json subMapping = mappingJson;

            do {
                std::string::size_type slashPosition = remainingTopic.find("/");

                std::string topicLevelName = remainingTopic.substr(0, slashPosition);
                remainingTopic.erase(0, topicLevelName.size() + 1);

                const nlohmann::json& topicLevels = subMapping.value("topic_level", nlohmann::json::array());

                if (topicLevels.is_array()) {
                    subMapping = *std::find_if(
                        topicLevels.begin(), topicLevels.end(), [&topicLevelName](const nlohmann::json& subMappingCandidat) -> bool {
                            return subMappingCandidat.contains("name") && subMappingCandidat["name"].is_string() &&
                                   subMappingCandidat["name"] == topicLevelName;
                        });
                } else if (topicLevels.is_object() && topicLevels.contains("name") && topicLevels["name"].is_string() &&
                           topicLevels["name"] == topicLevelName) {
                    subMapping = topicLevels;
                } else {
                    subMapping.clear();
                }
            } while ((!remainingTopic.empty()) && !subMapping.empty());

            if (remainingTopic.empty() && subMapping.is_object()) {
                const nlohmann::json& mapping = subMapping.value("mapping", nlohmann::json::object());

                if (mapping.contains("static")) {
                    const nlohmann::json& staticMapping = mapping["static"];

                    if (staticMapping.is_object() && staticMapping.contains("command_topic") &&
                        staticMapping.contains("message_mappings") && staticMapping["message_mappings"].is_array()) {
                        const std::string& commandTopic = staticMapping["command_topic"];
                        bool retain = staticMapping.value("retain_message", false);
                        const nlohmann::json& messageMappingArray = staticMapping["message_mappings"];

                        const nlohmann::json& messageMapping =
                            *std::find_if(messageMappingArray.begin(),
                                          messageMappingArray.end(),
                                          [&publish](const nlohmann::json& messageMappingCandidat) {
                                              return messageMappingCandidat.is_object() &&
                                                     messageMappingCandidat.value("message", "") == publish.getMessage();
                                          });

                        if (messageMapping.is_object() && messageMapping.contains("mapped_message")) {
                            const std::string& message = messageMapping["mapped_message"];

                            LOG(INFO) << "Topic mapping found:";
                            LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << commandTopic << ":"
                                      << message;

                            publishMapping(commandTopic, message, publish.getQoS(), retain);
                        }
                    }
                } else {
                    try {
                        nlohmann::json json;
                        nlohmann::json templateMapping;

                        if (mapping.contains("value")) {
                            templateMapping = mapping["value"];

                            json["value"] = publish.getMessage();
                        } else if (mapping.contains("json")) {
                            templateMapping = mapping["json"];

                            json["json"] = nlohmann::json::parse(publish.getMessage());
                        }

                        if (!json.empty()) {
                            LOG(INFO) << "Topic mapping found:";
                        }

                        publishTemplates(templateMapping, json, publish);
                    } catch (const nlohmann::json::exception& e) {
                        LOG(ERROR) << e.what();
                    }
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
            "topic_level" : [
                {
                    "name" : "test01",
                    "topic_level" : [
                        {
                            "name" : "button1",
                            "type" : "binary_sensor",
                            "subscribtion" : {
                                "qos" : 2
                            },
                            "mapping": {
                                "Comment" : "Either a \"static\", \"value\", or \"json\" object is required! E.g. next three objects.
    Exclusive order of interpretation: \"static\", \"value\", \"json\"", "value" : { "command_topic" : "test02/onboard/set",
                                    "retain_message" : false,
                                    "mapping_template" : "{% if value == \"pressed\" %}on{% else if value == \"released\" %}off{% endif %}"
                                }
                            }
                        }
                    ]
                },{
                    "name" : "test03",
                    "topic_level" : [
                        {
                            "name" : "button1",
                            "type" : "binary_sensor",
                            "subscribtion" : {
                                "qos" : 2
                            },
                            "mapping": {
                                "Comment" : "Either a \"static\", \"value\", or \"json\" object is required! E.g. next three objects.
    Exclusive order of interpretation: \"static\", \"value\", \"json\"", "value" : { "command_topic" : "test02/onboard/set",
                                    "retain_message" : false,
                                    "mapping_template" : "{% if value == \"pressed\" %}on{% else if value == \"released\" %}off{% endif %}"
                                }
                            }
                        },{
                            "name" : "button2",
                            "type" : "binary_sensor",
                            "subscribtion" : {
                                "qos" : 2
                            },
                            "mapping": {
                                "Comment" : "Either a \"static\", \"value\", or \"json\" object is required! E.g. next three objects.
    Exclusive order of interpretation: \"static\", \"value\", \"json\"", "static" : { "command_topic" : "test02/onboard/set",
                                    "retain_message" : false,
                                    "message_mappings" : [
                                        {
                                            "message" : "pressed",
                                            "mapped_message" : "on"
                                        },{
                                            "message" : "released",
                                            "mapped_message" : "off"
                                        }
                                    ]
                                },
                                "The next objects are ignored" : "Listed only for documentation",
                                "value | Comment: An array of \"value\" objects is also allowed for multi-mapping. E.g.:" : [
                                    {
                                        "command_topic" : "aCommandTopic",
                                        "retain_message" : false,
                                        "mapping_template" : "aStateTemplate"
                                    },
                                    {
                                        "command_topic" : "anOtherCommandTopic",
                                        "retain_message" : false,
                                        "mapping_template" : "anOtherStateTemplate"
                                    }

                                ],
                                "json | Comment: An array of \"json\" objects is also allowed for multi-mapping. E.g.:" : [
                                    {
                                        "command_topic" : "aCommandTopic",
                                        "retain_message" : false,
                                        "mapping_template" : "aStateTemplate"
                                    },
                                    {
                                        "command_topic" : "anOtherCommandTopic",
                                        "retain_message" : false,
                                        "mapping_template" : "anOtherStateThemplate"
                                    }
                                ]
                            }
                        },{
                            "name" : "button3",
                            "type" : "binary_sensor",
                            "subscribtion" : {
                                "qos" : 2
                            },
                            "mapping": {
                                "Comment" : "Either a \"static\", \"value\", or \"json\" object is required! E.g. next three objects.
    Exclusive order of interpretation: \"static\", \"value\", \"json\"", "json" : { "command_topic" : "test02/onboard/set", "retain_message"
    : false, "mapping_template" : "{{ json.time.start }} to {{ json.time.end + 1 }}pm"
                                }
                            }
                        }
                    ]
                }
            ]
        }
    }

    */

} // namespace apps::mqttbroker::lib
