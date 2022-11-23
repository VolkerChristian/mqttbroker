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

#include "nlohmann/json-schema.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <fstream>
#include <initializer_list>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::lib {

    nlohmann::json JsonMappingReader::mappingJson;

    static void loader(const nlohmann::json_uri& uri, nlohmann::json& schema) {
        std::string filename = "./" + uri.path();
        std::ifstream lf(filename);
        if (!lf.good())
            throw std::invalid_argument("could not open " + uri.url() + " tried with " + filename);
        try {
            lf >> schema;
        } catch (const std::exception& e) {
            throw;
        }
    }

    class custom_error_handler : public nlohmann::json_schema::basic_error_handler {
        void error(const nlohmann::json::json_pointer& ptr, const nlohmann::json& instance, const std::string& message) override {
            nlohmann::json_schema::basic_error_handler::error(ptr, instance, message);
            std::cerr << "ERROR: '" << ptr << "' - '" << instance << "': " << message << "\n";
        }
    };

    const nlohmann::json& JsonMappingReader::readMappingFromFile(const std::string& mappingFilePath) {
        if (mappingJson.empty()) {
            std::ifstream mappingFile(mappingFilePath);

            if (mappingFile.good()) {
                VLOG(0) << "MappingFilePath: " << mappingFilePath;

                try {
                    try {
                        mappingFile >> mappingJson;
                    } catch (const std::exception& e) {
                        std::cerr << e.what() << " at " << mappingFile.tellg() << " - while parsing the json mapping file\n";
                        exit(0);
                    }

                    nlohmann::json_schema::json_validator validator(loader, nlohmann::json_schema::default_string_format_check);

                    custom_error_handler err;
                    try {
                        validator.set_root_schema(mappingJsonSchema);
                    } catch (const std::exception& e) {
                        VLOG(0) << "Setting root schema failed";
                        VLOG(0) << e.what();
                    }

                    nlohmann::json defaultPatch = validator.validate(mappingJson, err);
                    VLOG(0) << defaultPatch.dump(2);
                    mappingJson = mappingJson.patch(defaultPatch);
                } catch (const nlohmann::json::exception& e) {
                    LOG(ERROR) << e.what();
                }
            } else {
                VLOG(0) << "MappingFilePath: " << mappingFilePath << " not found";
            }
            mappingFile.close();
        } else {
            VLOG(0) << "MappingFilePath: " << mappingFilePath << " already loaded";
        }

        return mappingJson;
    }

    nlohmann::json JsonMappingReader::mappingJsonSchema =
#include "mapping-json-schema.json"

} // namespace apps::mqttbroker::lib
