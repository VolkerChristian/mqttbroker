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

#include "MqttSubProtocolFactory.h"

#include <iot/mqtt/server/broker/Broker.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <initializer_list>
#include <log/Logger.h>
#include <map>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define NAME "mqtt"

namespace web::websocket::subprotocol::echo::server {

    MqttSubprotocolFactory::MqttSubprotocolFactory(const std::string& name)
        : web::websocket::SubProtocolFactory<MqttSubProtocol>::SubProtocolFactory(name)
        , mappingJson(nlohmann::json()) {
        char* json = getenv("MQTT_MAPPING_JSON");

        if (json != nullptr) {
            mappingJson = nlohmann::json::parse(json);
            VLOG(0) << "DUMP: " << mappingJson.dump(4);
        }
    }

    MqttSubProtocol* MqttSubprotocolFactory::create(SubProtocolContext* subProtocolContext) {
        return new MqttSubProtocol(
            subProtocolContext, getName(), iot::mqtt::server::broker::Broker::instance(SUBSCRIBTION_MAX_QOS), mappingJson);
    }

} // namespace web::websocket::subprotocol::echo::server

extern "C" void* mqttServerSubProtocolFactory() {
    return new web::websocket::subprotocol::echo::server::MqttSubprotocolFactory(NAME);
}
