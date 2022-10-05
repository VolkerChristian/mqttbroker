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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Timeval.h" // IWYU pragma: keep

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection, std::shared_ptr<Broker> broker)
        : iot::mqtt::SocketContext(socketConnection)
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

    void SocketContext::onConnect(const iot::mqtt::packets::Connect& connect) {
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
        LOG(DEBUG) << "Error: " << connect.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(connect.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(connect.getFlags());
        LOG(DEBUG) << "RemainingLength: " << connect.getRemainingLength();
        LOG(DEBUG) << "Protocol: " << protocol;
        LOG(DEBUG) << "Version: " << static_cast<uint16_t>(level);
        LOG(DEBUG) << "ConnectFlags: " << static_cast<uint16_t>(connectFlags);
        LOG(DEBUG) << "KeepAlive: " << keepAlive;
        if (keepAlive != 0) {
            setTimeout(1.5 * keepAlive);
        } else {
            // Leave the read- and write-timeouts as configured for this SocketContext (default 60 sec)
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

    void SocketContext::onConnack(const iot::mqtt::packets::Connack& connack) {
        LOG(DEBUG) << "CONNACK";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << connack.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(connack.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(connack.getFlags());
        LOG(DEBUG) << "RemainingLength: " << connack.getRemainingLength();
        LOG(DEBUG) << "Flags: " << static_cast<uint16_t>(connack.getFlags());
        LOG(DEBUG) << "Reason: " << connack.getReturnCode();
    }

    void SocketContext::onPublish(const iot::mqtt::packets::Publish& publish) {
        LOG(DEBUG) << "PUBLISH";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << publish.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(publish.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(publish.getFlags());
        LOG(DEBUG) << "RemainingLength: " << publish.getRemainingLength();
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

    void SocketContext::onPuback(const iot::mqtt::packets::Puback& puback) {
        LOG(DEBUG) << "PUBACK";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << puback.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(puback.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(puback.getFlags());
        LOG(DEBUG) << "RemainingLength: " << puback.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << puback.getPacketIdentifier();
    }

    void SocketContext::onPubrec(const iot::mqtt::packets::Pubrec& pubrec) {
        LOG(DEBUG) << "PUBREC";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << pubrec.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pubrec.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pubrec.getFlags());
        LOG(DEBUG) << "RemainingLength: " << pubrec.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << pubrec.getPacketIdentifier();
    }

    void SocketContext::onPubrel(const iot::mqtt::packets::Pubrel& pubrel) {
        LOG(DEBUG) << "PUBREL";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << pubrel.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pubrel.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pubrel.getFlags());
        LOG(DEBUG) << "RemainingLength: " << pubrel.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << pubrel.getPacketIdentifier();
    }

    void SocketContext::onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) {
        LOG(DEBUG) << "PUBCOMP";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << pubcomp.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pubcomp.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pubcomp.getFlags());
        LOG(DEBUG) << "RemainingLength: " << pubcomp.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << pubcomp.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) {
        LOG(DEBUG) << "SUBSCRIBE";
        LOG(DEBUG) << "=========";
        LOG(DEBUG) << "Error: " << subscribe.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(subscribe.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(subscribe.getFlags());
        LOG(DEBUG) << "RemainingLength: " << subscribe.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic.getName() << ", requestedQoS: " << static_cast<uint16_t>(topic.getRequestedQoS());
            broker->subscribe(topic.getName(), clientId, topic.getRequestedQoS());
        }
    }

    void SocketContext::onSuback(const iot::mqtt::packets::Suback& suback) {
        LOG(DEBUG) << "SUBACK";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << suback.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(suback.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(suback.getFlags());
        LOG(DEBUG) << "RemainingLength: " << suback.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << suback.getPacketIdentifier();

        for (uint8_t returnCode : suback.getReturnCodes()) {
            LOG(DEBUG) << "  Return Code: " << static_cast<uint16_t>(returnCode);
        }
    }

    void SocketContext::onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) {
        LOG(DEBUG) << "UNSUBSCRIBE";
        LOG(DEBUG) << "===========";
        LOG(DEBUG) << "Error: " << unsubscribe.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(unsubscribe.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(unsubscribe.getFlags());
        LOG(DEBUG) << "RemainingLength: " << unsubscribe.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic;
            broker->unsubscribe(topic, clientId);
        }
    }

    void SocketContext::onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) {
        LOG(DEBUG) << "UNSUBACK";
        LOG(DEBUG) << "========";
        LOG(DEBUG) << "Error: " << unsuback.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(unsuback.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(unsuback.getFlags());
        LOG(DEBUG) << "RemainingLength: " << unsuback.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << unsuback.getPacketIdentifier();
    }

    void SocketContext::onPingreq(const iot::mqtt::packets::Pingreq& pingreq) {
        LOG(DEBUG) << "PINGREQ";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << pingreq.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pingreq.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pingreq.getFlags());
        LOG(DEBUG) << "RemainingLength: " << pingreq.getRemainingLength();
    }

    void SocketContext::onPingresp(const iot::mqtt::packets::Pingresp& pingresp) {
        LOG(DEBUG) << "PINGRESP";
        LOG(DEBUG) << "========";
        LOG(DEBUG) << "Error: " << pingresp.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pingresp.getType());
        LOG(DEBUG) << "RemainingLength: " << pingresp.getRemainingLength();
    }

    void SocketContext::onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) {
        LOG(DEBUG) << "DISCONNECT";
        LOG(DEBUG) << "==========";
        LOG(DEBUG) << "Error: " << disconnect.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(disconnect.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(disconnect.getFlags());
        LOG(DEBUG) << "RemainingLength: " << disconnect.getRemainingLength();

        willFlag = false;

        releaseSession();
    }

} // namespace mqtt::broker
