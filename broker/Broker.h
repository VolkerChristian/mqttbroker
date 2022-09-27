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

#ifndef MQTT_SERVER_BROKER_H
#define MQTT_SERVER_BROKER_H

#include "broker/SubscriberTree.h"
#include "broker/TopicTree.h"

namespace mqtt::broker {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    class Broker {
    private:
        Broker();

    public:
        static Broker& instance();

        ~Broker();

        void subscribe(const std::string& topic, mqtt::broker::SocketContext* socketContext, uint8_t qoSLevel);
        void publish(const std::string& topic, const std::string& message);
        void unsubscribe(const std::string& topic, mqtt::broker::SocketContext* socketContext);
        void unsubscribe(mqtt::broker::SocketContext* socketContext);

    private:
        mqtt::broker::SubscriberTree subscriberTree;
        mqtt::broker::TopicTree topicTree;
    };

} // namespace mqtt::broker

#endif // SERVER_BROKER_H
