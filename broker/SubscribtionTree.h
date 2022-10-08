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

#ifndef APPS_MQTT_SERVER_SUBSCRIBERTREE_H
#define APPS_MQTT_SERVER_SUBSCRIBERTREE_H

namespace mqtt::broker {
    class Broker;
} // namespace mqtt::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    class SubscribtionTree {
    public:
        explicit SubscribtionTree(mqtt::broker::Broker* broker);

        void publishRetained(const std::string& clientId);

        bool subscribe(const std::string& fullTopicName, const std::string& clientId, uint8_t clientQoSLevel);

        void publish(const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel, bool retained);

        bool unsubscribe(std::string fullTopicName, const std::string& clientId);
        bool unsubscribe(const std::string& clientId);

    private:
        class SubscribtionTreeNode {
        public:
            explicit SubscribtionTreeNode(mqtt::broker::Broker* broker);

            void publishRetained(const std::string& clientId);

            bool subscribe(const std::string& fullTopicName,
                           const std::string& clientId,
                           uint8_t clientQoSLevel,
                           std::string remainingTopicName,
                           bool leafFound);

            void publish(const std::string& fullTopicName,
                         const std::string& message,
                         uint8_t qoSLevel,
                         bool retained,
                         std::string remainingTopicName,
                         bool leafFound);

            bool unsubscribe(const std::string& clientId, std::string remainingTopicName, bool leafFound);
            bool unsubscribe(const std::string& clientId);

        private:
            std::map<std::string, uint8_t> subscribers;
            std::map<std::string, SubscribtionTreeNode> subscribtions;

            std::string subscribedTopicName = "";

            mqtt::broker::Broker* broker;
        };

        SubscribtionTreeNode head;
    };

} // namespace mqtt::broker

#endif // APPS_MQTT_SERVER_SUBSCRIBERTREE_H
