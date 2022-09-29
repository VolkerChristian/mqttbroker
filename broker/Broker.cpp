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

#include <algorithm>
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

    void Broker::subscribe(const std::string& topic, const std::string& clientId, uint8_t suscribedQoSLevel) {
        subscribtionTree.subscribe(topic, clientId, suscribedQoSLevel);

        retainTree.publish(topic, clientId, suscribedQoSLevel);
    }

    void Broker::publish(const std::string& topic, const std::string& message, uint8_t qoSLevel, bool retain) {
        subscribtionTree.publish(topic, message, qoSLevel, retain);
    }

    void Broker::retain(const std::string& topic, const std::string& message, uint8_t qoSLevel) {
        retainTree.retain(topic, message, qoSLevel);
    }

    void Broker::unsubscribe(const std::string& topic, const std::string& clientId) {
        subscribtionTree.unsubscribe(topic, clientId);
    }

    void Broker::unsubscribe(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
    }

    bool Broker::hasSession(const std::string& clientId) {
        return sessions.contains(clientId);
    }

    void Broker::newSession(const std::string& clientId, SocketContext* socketContext) {
        LOG(TRACE) << "New session: " << clientId << " - " << socketContext;

        sessions[clientId] = socketContext;
    }

    void Broker::renewSession(const std::string& clientId, SocketContext* socketContext) {
        LOG(TRACE) << "Attach session: " << clientId << " - " << socketContext;

        sessions[clientId] = socketContext;
        subscribtionTree.publishRetained(clientId);

        // TODO: send queued messages
    }

    void Broker::retainSession(const std::string& clientId) {
        LOG(TRACE) << "Retain session: " << clientId;

        sessions[clientId] = nullptr;
    }

    void Broker::deleteSession(const std::string& clientId) {
        LOG(TRACE) << "Delete session: " << clientId;

        sessions.erase(clientId);
    }

    void Broker::sendPublish(const std::string& clientId,
                             const std::string& fullTopicName,
                             const std::string& message,
                             bool dup,
                             uint8_t qoSLevel,
                             bool retain,
                             uint8_t clientQoSLevel) {
        if (sessions.contains(clientId)) {
            if (sessions[clientId] != nullptr) { // client online
                LOG(TRACE) << "Send Publish: " << clientId << ": " << fullTopicName << " - " << message << " - "
                           << static_cast<uint16_t>(std::min(clientQoSLevel, qoSLevel));

                sessions[clientId]->sendPublish(fullTopicName, message, dup, std::min(qoSLevel, clientQoSLevel), retain);
            } else if (qoSLevel > 0) { // only for QoS = 1 and 2
                // Queue Messages for offline clients
            } else {
                // Discard all queued messages and use current message as the only one queued message
            }
        }
    }

    void Broker::sendRetained(const std::string& topic, const std::string& clientId, uint8_t clientQoSLevel) {
        retainTree.publish(topic, clientId, clientQoSLevel);
    }

    SocketContext* Broker::getSocketContext(const std::string& clientId) {
        return sessions[clientId];
    }

    std::string Broker::getRandomClientId() {
        char uuid[UUID_LEN];

        std::ifstream file("/proc/sys/kernel/random/uuid");
        file.getline(uuid, UUID_LEN);
        file.close();

        return std::string(uuid);
    }

} // namespace mqtt::broker
