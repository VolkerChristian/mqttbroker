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

#include "broker/SocketContext.h"

#include "broker/Broker.h"
// #include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Timeval.h" // IWYU pragma: keep

#include <iomanip>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection, std::shared_ptr<Broker> broker)
        : iot::mqtt::server::SocketContext(socketConnection)
        , broker(broker) {
    }

    SocketContext::~SocketContext() {
        releaseSession();

        if (willFlag) {
            broker->publish(willTopic, willMessage, willQoS, willRetain);
        }
    }

    void SocketContext::initSession() {
        if (broker->hasActiveSession(clientId)) {
            LOG(TRACE) << "ClientID \'" << clientId << "\' already in use ... disconnecting";

            close();
        } else if (broker->hasRetainedSession(clientId)) {
            LOG(TRACE) << "ClientId \'" << clientId << "\' found retained session ... renewing";
            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_PRESENT);

            if (cleanSession) {
                LOG(TRACE) << "CleanSession ... discarding subscribtions";
                broker->unsubscribe(clientId);
            } else {
                LOG(TRACE) << "RetainSession ... retaining subscribtions";
            }

            broker->renewSession(clientId, this);
        } else {
            LOG(TRACE) << "ClientId \'" << clientId << "\' no existing session ... creating";
            sendConnack(MQTT_CONNACK_ACCEPT, MQTT_SESSION_NEW);

            broker->newSession(clientId, this);
        }
    }

    void SocketContext::releaseSession() {
        if (cleanSession) {
            broker->deleteSession(clientId, this);
        } else {
            broker->retainSession(clientId, this);
        }
    }

    void SocketContext::onConnect(iot::mqtt::server::packets::Connect& connect) {
        // V-Header
        protocol = connect.getProtocol();
        level = connect.getLevel();
        connectFlags = connect.getConnectFlags();
        keepAlive = connect.getKeepAlive();

        // Payload
        clientId = connect.getClientId();
        willTopic = connect.getWillTopic();
        willMessage = connect.getWillMessage();
        username = connect.getUsername();
        password = connect.getPassword();

        // Derived from flags
        usernameFlag = connect.getUsernameFlag();
        passwordFlag = connect.getPasswordFlag();
        willRetain = connect.getWillRetain();
        willQoS = connect.getWillQoS();
        willFlag = connect.getWillFlag();
        cleanSession = connect.getCleanSession();

        LOG(DEBUG) << "CONNECT";
        LOG(DEBUG) << "=======";
        printStandardHeader(connect);
        LOG(DEBUG) << "Protocol: " << protocol;
        LOG(DEBUG) << "Version: " << static_cast<uint16_t>(level);
        LOG(DEBUG) << "ConnectFlags: " << static_cast<uint16_t>(connectFlags);
        LOG(DEBUG) << "KeepAlive: " << keepAlive;
        if (keepAlive != 0) {
            setTimeout(1.5 * keepAlive);
        } else {
            // setTimeout(core::DescriptorEventReceiver::TIMEOUT::DISABLE); // Leaf at framework default (default 60 sec)
        }
        LOG(DEBUG) << "ClientID: " << clientId;
        LOG(DEBUG) << "CleanSession: " << cleanSession;
        if (willFlag) {
            LOG(DEBUG) << "WillTopic: " << willTopic;
            LOG(DEBUG) << "WillMessage: " << willMessage;
            LOG(DEBUG) << "WillQoS: " << static_cast<uint16_t>(willQoS);
            LOG(DEBUG) << "WillRetain: " << willRetain;
        }
        if (usernameFlag) {
            LOG(DEBUG) << "Username: " << username;
        }
        if (passwordFlag) {
            LOG(DEBUG) << "Password: " << password;
        }

        initSession();
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        LOG(DEBUG) << "PUBLISH";
        LOG(DEBUG) << "=======";
        printStandardHeader(publish);
        LOG(DEBUG) << "DUP: " << publish.getDup();
        LOG(DEBUG) << "QoSLevel: " << static_cast<uint16_t>(publish.getQoSLevel());
        LOG(DEBUG) << "Retain: " << publish.getRetain();
        LOG(DEBUG) << "Topic: " << publish.getTopic();
        LOG(DEBUG) << "PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "Message: " << publish.getMessage();

        broker->publish(publish.getTopic(), publish.getMessage(), publish.getQoSLevel());
        if (publish.getRetain()) {
            broker->retain(publish.getTopic(), publish.getMessage(), publish.getQoSLevel());
        }
    }

    void SocketContext::onPuback(iot::mqtt::packets::Puback& puback) {
        LOG(DEBUG) << "PUBACK";
        LOG(DEBUG) << "======";
        printStandardHeader(puback);
        LOG(DEBUG) << "PacketIdentifier: " << puback.getPacketIdentifier();
    }

    void SocketContext::onPubrec(iot::mqtt::packets::Pubrec& pubrec) {
        LOG(DEBUG) << "PUBREC";
        LOG(DEBUG) << "======";
        printStandardHeader(pubrec);
        LOG(DEBUG) << "PacketIdentifier: " << pubrec.getPacketIdentifier();
    }

    void SocketContext::onPubrel(iot::mqtt::packets::Pubrel& pubrel) {
        LOG(DEBUG) << "PUBREL";
        LOG(DEBUG) << "======";
        printStandardHeader(pubrel);
        LOG(DEBUG) << "PacketIdentifier: " << pubrel.getPacketIdentifier();
    }

    void SocketContext::onPubcomp(iot::mqtt::packets::Pubcomp& pubcomp) {
        LOG(DEBUG) << "PUBCOMP";
        LOG(DEBUG) << "=======";
        printStandardHeader(pubcomp);
        LOG(DEBUG) << "PacketIdentifier: " << pubcomp.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(iot::mqtt::server::packets::Subscribe& subscribe) {
        LOG(DEBUG) << "SUBSCRIBE";
        LOG(DEBUG) << "=========";
        printStandardHeader(subscribe);
        LOG(DEBUG) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        for (iot::mqtt::Topic& topic : subscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic.getName() << ", requestedQoS: " << static_cast<uint16_t>(topic.getRequestedQoS());
            uint8_t ret = broker->subscribe(topic.getName(), clientId, topic.getRequestedQoS());
            topic.setAcceptedQoS(ret);
        }
    }

    void SocketContext::onUnsubscribe(iot::mqtt::server::packets::Unsubscribe& unsubscribe) {
        LOG(DEBUG) << "UNSUBSCRIBE";
        LOG(DEBUG) << "===========";
        printStandardHeader(unsubscribe);
        LOG(DEBUG) << "PacketIdentifier: " << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic;
            broker->unsubscribe(topic, clientId);
        }
    }

    void SocketContext::onPingreq(iot::mqtt::server::packets::Pingreq& pingreq) {
        LOG(DEBUG) << "PINGREQ";
        LOG(DEBUG) << "=======";
        printStandardHeader(pingreq);
    }

    void SocketContext::onDisconnect(iot::mqtt::server::packets::Disconnect& disconnect) {
        LOG(DEBUG) << "DISCONNECT";
        LOG(DEBUG) << "==========";
        printStandardHeader(disconnect);

        willFlag = false;

        releaseSession();
    }

    void SocketContext::printStandardHeader(const iot::mqtt::ControlPacketReceiver& packet) {
        LOG(DEBUG) << "Error: " << packet.isError();
        LOG(DEBUG) << "Type: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getType());
        LOG(DEBUG) << "Flags: 0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(packet.getFlags());
        LOG(DEBUG) << "RemainingLength: " << packet.getRemainingLength();
    }

} // namespace mqtt::broker
