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

#ifndef APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXTFACTORY_H
#define APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXTFACTORY_H

#include "SocketContext.h" // IWYU pragma: export

#include <core/socket/SocketContextFactory.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::integrator {

    template <const nlohmann::json& connectionT, const nlohmann::json& jsonMappingT>
    class SocketContextFactory : public core::socket::SocketContextFactory {
    public:
        SocketContextFactory();

        core::socket::SocketContext* create(core::socket::SocketConnection* socketConnection) override;

    private:
        const nlohmann::json& connection;
        const nlohmann::json& jsonMapping;
    };

} // namespace apps::mqttbroker::integrator

#endif // APPS_MQTTBROKER_MQTTINTEGRATOR_SOCKETCONTEXTFACTORY_H
