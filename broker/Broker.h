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

#ifndef APPS_MQTT_SERVER_BROKER_H
#define APPS_MQTT_SERVER_BROKER_H

#include "broker/RetainTree.h"
#include "broker/SubscribtionTree.h"

namespace mqtt::broker {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    class Broker {
    private:
    public:
        Broker();

        static std::shared_ptr<Broker> instance();

        void subscribe(const std::string& topic, const std::string& clientId, uint8_t qoSLevel);
        void publish(const std::string& topic, const std::string& message, bool retain);
        void unsubscribe(const std::string& topic, const std::string& clientId);
        void unsubscribe(const std::string& clientId);

        bool hasSession(const std::string& clientId);
        void newSession(const std::string& clientId, mqtt::broker::SocketContext* socketContext);
        void renewSession(const std::string& clientId, mqtt::broker::SocketContext* socketContext);
        void retainSession(const std::string& clientId);
        void deleteSession(const std::string& clinetId);

        void sendPublish(const std::string& clientId,
                         const std::string& fullTopicName,
                         const std::string& message,
                         bool dup,
                         uint8_t qoSLevel,
                         bool retain);

        void sendRetained(const std::string& clientId, const std::string& topicName, uint8_t qoSLevel);

        mqtt::broker::SocketContext* getSessionContext(const std::string& clientId);

        std::string getRandomClientUUID();

    private:
        mqtt::broker::SubscribtionTree subscribtionTree;
        mqtt::broker::RetainTree retainTree;

        std::map<std::string, mqtt::broker::SocketContext*> sessions;

        static std::shared_ptr<Broker> broker;
    };

} // namespace mqtt::broker

#endif // APPS_MQTT_SERVER_BROKER_H
