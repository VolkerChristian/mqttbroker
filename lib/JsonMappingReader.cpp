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

#include "JsonMappingReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <fstream>
#include <initializer_list>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::lib {

    nlohmann::json JsonMappingReader::jsonMapping;

    const nlohmann::json& JsonMappingReader::readMappingFromFile(const std::string& mappingFilePath) {
        if (jsonMapping.empty()) {
            std::ifstream mappingFile(mappingFilePath);
            if (mappingFile) {
                VLOG(0) << "MappingFilePath: " << mappingFilePath;

                jsonMapping = nlohmann::json::parse(mappingFile);
            } else {
                VLOG(0) << "MappingFilePath: " << mappingFilePath << " not found";
            }
            mappingFile.close();
        } else {
            VLOG(0) << "MappingFilePath: " << mappingFilePath << " already loaded";
        }

        return jsonMapping;
    }

} // namespace apps::mqttbroker::lib
