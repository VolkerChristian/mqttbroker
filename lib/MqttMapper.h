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

#ifndef APPS_MQTTBROKER_LIB_MQTTMAPPER_H
#define APPS_MQTTBROKER_LIB_MQTTMAPPER_H

namespace iot::mqtt {
    class Topic;
    namespace packets {
        class Publish;
    }
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <list>
#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::lib {

    class MqttMapper {
    public:
        MqttMapper(const nlohmann::json& mappingJson);

        virtual ~MqttMapper() = default;

    protected:
        std::string dump();

        std::list<iot::mqtt::Topic> extractTopics();
        void publishMappings(const iot::mqtt::packets::Publish& publish);

    private:
        static void extractTopics(const nlohmann::json& json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList);

        void publishTemplate(const nlohmann::json& mappingSubJson, const nlohmann::json& json, const iot::mqtt::packets::Publish& publish);
        void publishTemplates(const nlohmann::json& mappingSubJson, const nlohmann::json& json, const iot::mqtt::packets::Publish& publish);

        virtual void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS) = 0;

    protected:
        const nlohmann::json& mappingJson;
    };

} // namespace apps::mqttbroker::lib

#endif // APPS_MQTTBROKER_LIB_MQTTMAPPER_H
