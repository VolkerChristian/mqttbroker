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

#include "broker/Broker.h"

#include "broker/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <fstream>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#define UUID_LEN 36

namespace mqtt::broker {

    std::shared_ptr<Broker> Broker::broker;

    Broker::Broker()
        : subscribtionTree(this)
        , retainTree(this) {
    }

    std::shared_ptr<Broker> Broker::instance() {
        if (!broker) {
            broker = std::make_shared<Broker>();
        }

        return broker;
    }

    void Broker::subscribe(const std::string& topic, const std::string& clientId, uint8_t qoSLevel) {
        subscribtionTree.subscribe(topic, clientId, qoSLevel);

        retainTree.publish(topic, clientId, qoSLevel);
    }

    void Broker::publish(const std::string& topic, const std::string& message, bool retain) {
        subscribtionTree.publish(topic, message);

        if (retain) {
            retainTree.retain(topic, message);
        }
    }

    void Broker::unsubscribe(const std::string& topic, const std::string& clientId) {
        subscribtionTree.unsubscribe(topic, clientId);
    }

    void Broker::unsubscribe(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
    }

    void Broker::newSession(const std::string& clientId, SocketContext* socketContext) {
        VLOG(0) << "New Session: " << clientId << " - " << socketContext;
        sessions[clientId] = socketContext;
    }

    void Broker::replaceSession(const std::string& clientId, SocketContext* socketContext) {
        VLOG(0) << "Replace Session: " << clientId << " - " << socketContext;
        sessions[clientId] = socketContext;

        subscribtionTree.publishRetained(clientId);
    }

    void Broker::deleteSession(const std::string& clientId) {
        VLOG(0) << "DELETE SESSION: " << clientId;
        sessions.erase(clientId);
    }

    bool Broker::hasSession(const std::string& clientId) {
        return sessions.contains(clientId);
    }

    void Broker::sendPublish(const std::string& clientId,
                             const std::string& fullTopicName,
                             const std::string& message,
                             bool dup,
                             uint8_t qoSLevel,
                             bool retain) {
        if (sessions.contains(clientId) && sessions[clientId] != nullptr) {
            LOG(TRACE) << "Send Publich: " << clientId << ": " << fullTopicName << " - " << message << " - "
                       << static_cast<uint16_t>(qoSLevel);

            sessions[clientId]->sendPublish(fullTopicName, message, dup, qoSLevel, retain);
        }
    }

    void Broker::sendRetained(const std::string& clientId, const std::string& topicName, uint8_t qoSLevel) {
        retainTree.publish(topicName, clientId, qoSLevel);
    }

    SocketContext* Broker::getSessionContext(const std::string& clientId) {
        return sessions[clientId];
    }

    std::string Broker::getRandomClientUUID() {
        char uuidCharArray[UUID_LEN];
        std::ifstream file("/proc/sys/kernel/random/uuid");
        file.getline(uuidCharArray, UUID_LEN);
        file.close();
        return std::string(uuidCharArray);
    }

} // namespace mqtt::broker
