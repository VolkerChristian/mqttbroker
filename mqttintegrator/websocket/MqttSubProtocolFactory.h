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

#ifndef WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOLFACTORY_H
#define WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOLFACTORY_H

#include "mqttintegrator/websocket/MqttSubProtocol.h"

namespace web::websocket {
    class SubProtocolContext;
}

#include <web/websocket/SubProtocolFactory.h>

//

#include <nlohmann/json.hpp>
#include <string>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

namespace mqttbroker::mqttintegrator::websocket {

    class MqttSubprotocolFactory : public web::websocket::SubProtocolFactory<MqttSubProtocol> {
    public:
        explicit MqttSubprotocolFactory(const std::string& name);

    private:
        MqttSubProtocol* create(web::websocket::SubProtocolContext* subProtocolContext) override;

        nlohmann::json connection;
        nlohmann::json jsonMapping;
    };

} // namespace mqttbroker::mqttintegrator::websocket

extern "C" void* mqttClientSubProtocolFactory();

#endif // WEB_WEBSOCKET_SUBPROTOCOL_SERVER_MQTTSUBPROTOCOLFACTORY_H
