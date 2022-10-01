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

#include "broker/SubscribtionTree.h"

#include "broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>
#include <utility>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    SubscribtionTree::SubscribtionTree(Broker* broker)
        : broker(broker) {
    }

    void SubscribtionTree::publishRetainedMessages(const std::string& clientId) {
        if (subscribers.contains(clientId) && !subscribedTopicName.empty()) {
            broker->sendRetainedMessages(subscribedTopicName, clientId, subscribers[clientId]);
        }

        for (auto& [topicName, subscribtion] : subscribtions) {
            subscribtion.publishRetainedMessages(clientId);
        }
    }

    void SubscribtionTree::subscribe(const std::string& fullTopicName, const std::string& clientId, uint8_t clientQoSLevel) {
        subscribe(fullTopicName, fullTopicName, clientId, clientQoSLevel);
    }

    void SubscribtionTree::publish(const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel, bool retained) {
        publish(fullTopicName, fullTopicName, message, qoSLevel, retained);
    }

    bool SubscribtionTree::unsubscribe(const std::string& clientId) {
        subscribers.erase(clientId);

        for (auto it = subscribtions.begin(); it != subscribtions.end();) {
            if (it->second.unsubscribe(clientId)) {
                it = subscribtions.erase(it);
            } else {
                ++it;
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

    bool SubscribtionTree::unsubscribe(std::string remainingTopicName, const std::string& clientId) {
        if (remainingTopicName.empty()) {
            subscribers.erase(clientId);
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName) && subscribtions.find(topicName)->second.unsubscribe(remainingTopicName, clientId)) {
                subscribtions.erase(topicName);
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

    void SubscribtionTree::subscribe(std::string remainingTopicName,
                                     const std::string& fullTopicName,
                                     const std::string& clientId,
                                     uint8_t clientQoSLevel) {
        if (remainingTopicName.empty()) {
            subscribedTopicName = fullTopicName;
            subscribers[clientId] = clientQoSLevel;
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            subscribtions.insert({topicName, mqtt::broker::SubscribtionTree(broker)})
                .first->second.subscribe(remainingTopicName, fullTopicName, clientId, clientQoSLevel);
        }
    }

    void SubscribtionTree::publish(
        std::string remainingTopicName, const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel, bool retained) {
        if (remainingTopicName.empty()) {
            for (auto& [clientId, clientQoSLevel] : subscribers) {
                LOG(TRACE) << "Found Match: " << subscribedTopicName << " = " << fullTopicName << " -> " << message;
                LOG(TRACE) << "Distribute Publish ...";
                broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, retained, clientQoSLevel);
                LOG(TRACE) << "... completed!";
            }
        } else {
            std::string topicName = remainingTopicName.substr(0, remainingTopicName.find("/"));
            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName)) {
                subscribtions.find(topicName)->second.publish(remainingTopicName, fullTopicName, message, qoSLevel, retained);
            }
            if (subscribtions.contains("+")) {
                subscribtions.find("+")->second.publish(remainingTopicName, fullTopicName, message, qoSLevel, retained);
            }
            if (subscribtions.contains("#")) {
                const SubscribtionTree& foundSubscription = subscribtions.find("#")->second;

                LOG(TRACE) << "Found Match: " << subscribedTopicName << " = " << fullTopicName << " -> " << message;
                LOG(TRACE) << "Distribute Publish ...";
                for (auto& [clientId, clientQoSLevel] : foundSubscription.subscribers) {
                    broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, retained, clientQoSLevel);
                }
                LOG(TRACE) << "... completed!";
            }
        }
    }

} // namespace mqtt::broker
