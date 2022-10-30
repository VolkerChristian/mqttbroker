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

#ifndef APPS_MQTTBROKER_SOCKETCONTEXT_H
#define APPS_MQTTBROKER_SOCKETCONTEXT_H

#include "JsonMappingReader.h"
#include "iot/mqtt/server/SocketContext.h"

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt::server::broker {
    class Broker;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker {

    class SocketContext : public iot::mqtt::server::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection,
                               const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker);

        ~SocketContext() override;

    private:
        void onPublish(iot::mqtt::packets::Publish& publish) override;

        nlohmann::json& jsonMapping;
    };

} // namespace apps::mqttbroker

#endif // APPS_MQTTBROKER_SOCKETCONTEXT_H
