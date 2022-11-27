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
        std::string name = topicLevel["name"];

        if (topicLevel.contains("subscription")) {
            uint8_t qoS = topicLevel["subscription"]["qos"];

            topicList.push_back(iot::mqtt::Topic(topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, qoS));
        }

        if (topicLevel.contains("topic_level")) {
            extractTopics(topicLevel, topic + ((topic.empty() || topic == "/") && !name.empty() ? "" : "/") + name, topicList);
        }
    }

    void MqttMapper::extractTopics(const nlohmann::json& mappingJson, const std::string& topic, std::list<iot::mqtt::Topic>& topicList) {
        const nlohmann::json& topicLevels = mappingJson["topic_level"];

        if (topicLevels.is_array()) {
            for (const nlohmann::json& topicLevel : topicLevels) {
                extractTopic(topicLevel, topic, topicList);
            }
        } else if (topicLevels.is_object()) {
            extractTopic(topicLevels, topic, topicList);
        }
    }

    std::list<iot::mqtt::Topic> MqttMapper::extractTopics() {
        std::list<iot::mqtt::Topic> topicList;

        try {
            MqttMapper::extractTopics(mappingJson, "", topicList);
        } catch (const nlohmann::json::exception& e) {
            LOG(ERROR) << e.what();
            LOG(ERROR) << "Extracting topics failed.";
        }

        return topicList;
    }

    void MqttMapper::publishMappedTemplate(const nlohmann::json& templateMapping,
                                           const nlohmann::json& json,
                                           const iot::mqtt::packets::Publish& publish) {
        const std::string& commandTopic = templateMapping["mapped_topic"];
        const std::string& mappingTemplate = templateMapping["mapping_template"];

        try {
            // Render
            std::string renderedMessage = inja::render(mappingTemplate, json);

            LOG(INFO) << "      " << commandTopic << " : " << mappingTemplate << " -> '" << renderedMessage << "'";

            bool retain = templateMapping["retain_message"];
            uint8_t qoS = templateMapping.value("qos_override", publish.getQoS());

            publishMapping(commandTopic, renderedMessage, qoS, retain);
        } catch (const inja::InjaError& e) {
            LOG(ERROR) << e.what();
            LOG(ERROR) << "Rendering failed: " << mappingTemplate << " : " << json.dump();
        }
    }

    void MqttMapper::publishMappedTemplates(const nlohmann::json& templateMapping,
                                            const nlohmann::json& json,
                                            const iot::mqtt::packets::Publish& publish) {
        LOG(INFO) << "  " << publish.getTopic() << " : " << publish.getMessage() << " -> " << json.dump();

        if (!templateMapping.is_array()) {
            publishMappedTemplate(templateMapping, json, publish);
        } else {
            for (const nlohmann::json& concreteTemplateMapping : templateMapping) {
                publishMappedTemplate(concreteTemplateMapping, json, publish);
            }
        }
    }

    void MqttMapper::publishMappedMessage(const nlohmann::json& staticMapping,
                                          const nlohmann::json& messageMappingArray,
                                          const iot::mqtt::packets::Publish& publish) {
        const nlohmann::json::const_iterator matchedMessageMappingIterator =
            std::find_if(messageMappingArray.begin(), messageMappingArray.end(), [&publish](const nlohmann::json& messageMappingCandidat) {
                return messageMappingCandidat["message"] == publish.getMessage();
            });

        if (matchedMessageMappingIterator != messageMappingArray.end()) {
            const std::string& commandTopic = staticMapping["mapped_topic"];
            bool retain = staticMapping["retain_message"];
            uint8_t qoS = staticMapping.value("qos_override", publish.getQoS());

            const std::string& message = (*matchedMessageMappingIterator)["mapped_message"];

            LOG(INFO) << "Topic mapping (static) found:";
            LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << commandTopic << ":" << message;

            publishMapping(commandTopic, message, qoS, retain);
        }
    }

    void MqttMapper::publishMappedMessages(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish) {
        const nlohmann::json& messageMappingArray = staticMapping["message_mappings"];

        if (messageMappingArray.begin() != messageMappingArray.end()) {
            if (messageMappingArray.begin()->is_object()) {
                publishMappedMessage(staticMapping, messageMappingArray, publish);
            } else {
                for (const nlohmann::json& messageMappingArrayElement : messageMappingArray) {
                    publishMappedMessage(staticMapping, messageMappingArrayElement, publish);
                }
            }
        }
    }

    nlohmann::json MqttMapper::findMatchingTopicLevel(const iot::mqtt::packets::Publish& publish) {
        std::string remainingTopic = publish.getTopic();

        nlohmann::json matchedTopicLevel = mappingJson;

        do {
            std::string::size_type slashPosition = remainingTopic.find("/");

            std::string topicLevelName = remainingTopic.substr(0, slashPosition);
            remainingTopic.erase(0, topicLevelName.size() + 1);

            if (matchedTopicLevel.contains("topic_level")) {
                const nlohmann::json& topicLevels = matchedTopicLevel["topic_level"];

                nlohmann::json::const_iterator matchedTopicLevelIterator = std::find_if(
                    topicLevels.begin(), topicLevels.end(), [&topicLevelName](const nlohmann::json& topicLevelCandidat) -> bool {
                        return topicLevelCandidat["name"] == topicLevelName;
                    });

                if (matchedTopicLevelIterator != topicLevels.end()) {
                    matchedTopicLevel = *matchedTopicLevelIterator;
                } else {
                    matchedTopicLevel.clear();
                }
            } else {
                matchedTopicLevel.clear();
            }
        } while ((!remainingTopic.empty()) && !matchedTopicLevel.empty());

        return matchedTopicLevel;
    }

    void MqttMapper::publishMappings(const iot::mqtt::packets::Publish& publish) {
        nlohmann::json matchedTopicLevel = findMatchingTopicLevel(publish);

        if (!matchedTopicLevel.empty() && matchedTopicLevel.contains("subscription")) {
            const nlohmann::json& mapping = matchedTopicLevel["subscription"];

            if (mapping.contains("static")) {
                const nlohmann::json& staticMapping = mapping["static"];

                publishMappedMessages(staticMapping, publish);
            } else {
                nlohmann::json json;
                nlohmann::json templateMapping;

                if (mapping.contains("value")) {
                    LOG(INFO) << "Topic mapping (value) found:";

                    templateMapping = mapping["value"];

                    json["value"] = publish.getMessage();

                } else if (mapping.contains("json")) {
                    LOG(INFO) << "Topic mapping (json) found:";

                    templateMapping = mapping["json"];

                    try {
                        json = nlohmann::json::parse(publish.getMessage());
                    } catch (const nlohmann::json::exception& e) {
                        LOG(ERROR) << e.what();
                        LOG(ERROR) << "Parsing message into json failed: " << publish.getMessage();
                        json.clear();
                    }
                }

                if (!json.empty()) {
                    publishMappedTemplates(templateMapping, json, publish);
                } else {
                    LOG(INFO) << "No valid mapping section found: " << matchedTopicLevel.dump();
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
        "mapped_topic": "test02/onboard/set",
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
            "mapped_topic": "test02/onboard/set",
            "payload_on": "on",
            "payload_off": "off"
        }]
    }
    */

} // namespace apps::mqttbroker::lib
