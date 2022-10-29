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

#include <initializer_list>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker {

    nlohmann::json& JsonMappingReader::getMapping(const std::string& discoveryPrefix) {
        static nlohmann::json json;
        if (json.empty()) {
            json = nlohmann::json::parse(R"(
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
)");
        }

        return json[discoveryPrefix];
    }

} // namespace apps::mqttbroker
