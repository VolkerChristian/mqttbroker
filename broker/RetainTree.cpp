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

#include "broker/RetainTree.h"

#include "broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    RetainTree::RetainTree(Broker* broker)
        : broker(broker) {
    }

    void RetainTree::retain(const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel) {
        retain(fullTopicName, fullTopicName, message, qoSLevel, false);
    }

    void RetainTree::publish(std::string remainingTopicName, const std::string& clientId, uint8_t clientQoSLevel) {
        publish(remainingTopicName, clientId, clientQoSLevel, false);
    }

    bool RetainTree::retain(
        const std::string& fullTopicName, std::string remainingTopicName, const std::string& message, uint8_t qoSLevel, bool leafFound) {
        if (leafFound) {
            LOG(TRACE) << "Retaining: " << fullTopicName << " - " << message;
            this->fullTopicName = fullTopicName;
            this->message = message;
            this->qoSLevel = qoSLevel;
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTree.insert({topicName, RetainTree(broker)})
                    .first->second.retain(fullTopicName, remainingTopicName, message, qoSLevel, leafFound)) {
                topicTree.erase(topicName);
            }
        }

        return this->message.empty() && topicTree.empty();
    }

    void RetainTree::publish(std::string remainingTopicName, const std::string& clientId, uint8_t clientQoSLevel, bool leafFound) {
        if (leafFound) {
            if (!fullTopicName.empty()) {
                LOG(TRACE) << "Found retained message: " << fullTopicName << " - " << message << " - " << static_cast<uint16_t>(qoSLevel);
                LOG(TRACE) << "Distribute message ...";
                broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, RETAIN_TRUE, clientQoSLevel);
                LOG(TRACE) << "... completed!";
            }
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            if (topicTree.contains(topicName)) {
                topicTree.find(topicName)->second.publish(remainingTopicName, clientId, clientQoSLevel, leafFound);
            } else if (topicName == "+") {
                for (auto& topicTreeEntry : topicTree) {
                    topicTreeEntry.second.publish(remainingTopicName, clientId, clientQoSLevel, leafFound);
                }
            } else if (topicName == "#") {
                publish(clientId, clientQoSLevel);
            }
        }
    }

    void RetainTree::publish(const std::string& clientId, uint8_t clientQoSLevel) {
        if (!fullTopicName.empty()) {
            LOG(TRACE) << "Found retained message: " << fullTopicName << " - " << message << " - " << static_cast<uint16_t>(qoSLevel);
            LOG(TRACE) << "Distribute message ...";
            broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, RETAIN_TRUE, clientQoSLevel);
            LOG(TRACE) << "... completed!";
        }

        for (auto& [topicName, topicTree] : topicTree) {
            topicTree.publish(clientId, clientQoSLevel);
        }
    }

} // namespace mqtt::broker
