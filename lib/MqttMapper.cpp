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
        for (auto& [key, subJson] : json.items()) {
            if (subJson.is_object() && subJson.contains("payload")) {
                uint8_t qoS = 0;
                if (subJson.contains("subscribtion")) {
                    nlohmann::json subscribtion = subJson["subscribtion"];
                    if (subscribtion.is_object() && subscribtion.contains("qos")) {
                        qoS = subscribtion["qos"];
                    }
                }

                topicList.push_back(iot::mqtt::Topic(topic + (topic.empty() || topic == "/" ? "" : "/") + key, qoS));
            }
            if (key != "payload" && key != "qos" && subJson.is_object()) {
                extractTopics(subJson, topic + (topic.empty() || topic == "/" ? "" : "/") + key, topicList);
            }
        }
    }
    std::list<iot::mqtt::Topic> MqttMapper::extractTopics() {
        std::list<iot::mqtt::Topic> topicList;

        MqttMapper::extractTopics(jsonMapping, "", topicList);

        return topicList;
    }

    void MqttMapper::publishRenderedTemplate(const nlohmann::json& subJson,
                                             const nlohmann::json& json,
                                             const iot::mqtt::packets::Publish& publish) {
        if (subJson.contains("command_topic") && subJson.contains("state_template")) {
            const std::string& topic = subJson["command_topic"];
            const std::string& stateTemplate = subJson["state_template"];

            try {
                // Render
                std::string renderedMessage = inja::render(stateTemplate, json);

                publishMapping(topic, renderedMessage, publish.getQoS());
            } catch (const inja::InjaError& e) {
                LOG(ERROR) << e.what();
            }
        }
    }

    void
    MqttMapper::publishTemplate(const nlohmann::json& subJson, const nlohmann::json& json, const iot::mqtt::packets::Publish& publish) {
        if (subJson.is_object()) {
            publishRenderedTemplate(subJson, json, publish);
        } else if (subJson.is_array()) {
            for (const nlohmann::json& elementJson : subJson) {
                publishRenderedTemplate(elementJson, json, publish);
            }
        }
    }

    void MqttMapper::publishMappings(const iot::mqtt::packets::Publish& publish) {
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

            if (subJson.contains("static")) {
                subJson = subJson["static"];

                if (subJson.contains("command_topic") && subJson.contains(publish.getMessage())) {
                    const std::string& topic = subJson["command_topic"];
                    subJson = subJson[publish.getMessage()];

                    if (subJson.contains("state")) {
                        const std::string& message = subJson["state"];

                        LOG(INFO) << "Topic mapping found:";
                        LOG(INFO) << "  " << publish.getTopic() << ":" << publish.getMessage() << " -> " << topic << ":" << message;

                        publishMapping(topic, message, publish.getQoS());
                    }
                }
            } else if (subJson.contains("value")) {
                subJson = subJson["value"];

                try {
                    nlohmann::json json;
                    json["value"] = publish.getMessage();

                    publishTemplate(subJson, json, publish);
                } catch (const nlohmann::json::exception& e) {
                    LOG(ERROR) << e.what();
                }
            } else if (subJson.contains("json")) {
                subJson = subJson["json"];

                try {
                    nlohmann::json json;
                    json["json"] = nlohmann::json::parse(publish.getMessage());

                    publishTemplate(subJson, json, publish);
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
