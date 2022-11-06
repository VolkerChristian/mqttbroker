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

#include "core/timer/Timer.h"
#include "iot/mqtt/Topic.h" // IWYU pragma: export
#include "iot/mqtt/client/SocketContext.h"

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt {
    namespace packets {
        class Publish;
        class Connack;
    } // namespace packets
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker {

    class SocketContext : public iot::mqtt::client::SocketContext {
    public:
        explicit SocketContext(core::socket::SocketConnection* socketConnection, const nlohmann::json& jsonMapping);

        ~SocketContext() override;

    private:
        static std::list<iot::mqtt::Topic> extractTopics(nlohmann::json json, const std::string& topic);
        static void extractTopics(nlohmann::json json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList);

        void onConnected() override;
        void onExit() override;
        void onDisconnected() override;

        void onConnack(iot::mqtt::packets::Connack& connack) override;
        void onPublish(iot::mqtt::packets::Publish& publish) override;

        const nlohmann::json& jsonMapping;

        core::timer::Timer pingTimer;

        uint16_t packetIdentifier = 0;
    };

} // namespace apps::mqttbroker

#endif // APPS_MQTTBROKER_SOCKETCONTEXT_H
