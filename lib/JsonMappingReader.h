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

#ifndef APPS_MQTTBROKER_LIB_JSONMAPPINGREADER_H
#define APPS_MQTTBROKER_LIB_JSONMAPPINGREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::lib {

    class JsonMappingReader {
    private:
        JsonMappingReader() = delete;

    public:
        static const nlohmann::json readMappingFromFile(const std::string& mapFilePath);

        static const nlohmann::json& getConnectionJson();
        static const nlohmann::json& getMappingJson();

    private:
        static nlohmann::json mappingJsonSchema;
        static nlohmann::json connectionJson;
        static nlohmann::json mappingJson;
    };

} // namespace apps::mqttbroker::lib

#endif // APPS_MQTTBROKER_LIB_JSONMAPPINGREADER_H
