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

    void SubscribtionTree::publishRetained(const std::string& clientId) {
        if (subscribers.contains(clientId) && !subscribedTopicName.empty()) {
            broker->publishRetained(subscribedTopicName, clientId, subscribers[clientId]);
        }

        for (auto& [topicName, subscribtion] : subscribtions) {
            subscribtion.publishRetained(clientId);
        }
    }

    bool SubscribtionTree::subscribe(const std::string& fullTopicName, const std::string& clientId, uint8_t clientQoSLevel) {
        return subscribe(fullTopicName, fullTopicName, clientId, clientQoSLevel, false);
    }

    void SubscribtionTree::publish(const std::string& fullTopicName, const std::string& message, uint8_t qoSLevel, bool retained) {
        publish(fullTopicName, fullTopicName, message, qoSLevel, retained, false);
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
        return unsubscribe(remainingTopicName, clientId, false);
    }

    bool SubscribtionTree::unsubscribe(std::string remainingTopicName, const std::string& clientId, bool leafFound) {
        if (leafFound) {
            subscribers.erase(clientId);
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName) &&
                subscribtions.find(topicName)->second.unsubscribe(remainingTopicName, clientId, leafFound)) {
                subscribtions.erase(topicName);
            }
        }

        return subscribers.empty() && subscribtions.empty();
    }

    bool SubscribtionTree::subscribe(std::string remainingTopicName,
                                     const std::string& fullTopicName,
                                     const std::string& clientId,
                                     uint8_t clientQoSLevel,
                                     bool leafFound) {
        bool success = true;

        if (leafFound) {
            subscribedTopicName = fullTopicName;
            subscribers[clientId] = clientQoSLevel;
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            if (topicName == "#" && !remainingTopicName.ends_with("#")) {
                success = false;
            } else {
                remainingTopicName.erase(0, topicName.size() + 1);

                success = subscribtions.insert({topicName, mqtt::broker::SubscribtionTree(broker)})
                              .first->second.subscribe(remainingTopicName, fullTopicName, clientId, clientQoSLevel, leafFound);
            }
        }

        return success;
    }

    void SubscribtionTree::publish(std::string remainingTopicName,
                                   const std::string& fullTopicName,
                                   const std::string& message,
                                   uint8_t qoSLevel,
                                   bool retained,
                                   bool leafFound) {
        if (leafFound) {
            LOG(TRACE) << "Found Match: Subscribed Topic: '" << subscribedTopicName << "', Matched Topic: '" << fullTopicName
                       << "', Message: '" << message << "'";
            LOG(TRACE) << "Distribute Publish ...";
            for (auto& [clientId, clientQoSLevel] : subscribers) {
                broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, retained, clientQoSLevel);
            }
            LOG(TRACE) << "... completed!";
        } else {
            std::string::size_type slashPosition = remainingTopicName.find('/');

            std::string topicName = remainingTopicName.substr(0, slashPosition);
            bool leafFound = slashPosition == std::string::npos;

            remainingTopicName.erase(0, topicName.size() + 1);

            if (subscribtions.contains(topicName)) {
                subscribtions.find(topicName)->second.publish(remainingTopicName, fullTopicName, message, qoSLevel, retained, leafFound);
            }
            if (subscribtions.contains("+")) {
                subscribtions.find("+")->second.publish(remainingTopicName, fullTopicName, message, qoSLevel, retained, leafFound);
            }
            if (subscribtions.contains("#")) {
                const SubscribtionTree& foundSubscription = subscribtions.find("#")->second;

                LOG(TRACE) << "Found Match: Subscribed Topic: '" << foundSubscription.subscribedTopicName << "', Matched Topic: '"
                           << fullTopicName << "', Message: '" << message << "'";
                LOG(TRACE) << "Distribute Publish ...";
                for (auto& [clientId, clientQoSLevel] : foundSubscription.subscribers) {
                    broker->sendPublish(clientId, fullTopicName, message, DUP_FALSE, qoSLevel, retained, clientQoSLevel);
                }
                LOG(TRACE) << "... completed!";
            }
        }
    }

} // namespace mqtt::broker
