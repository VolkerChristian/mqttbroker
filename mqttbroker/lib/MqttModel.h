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

#ifndef MQTTBROKER_LIB_MQTTMODEL_H
#define MQTTBROKER_LIB_MQTTMODEL_H

namespace mqttbroker::broker::lib {
    class Mqtt;
}

#include <iot/mqtt/packets/Connect.h>

//

#include <map>

namespace mqttbroker::broker::lib {

    class MqttModel {
    private:
        MqttModel();

    public:
        static MqttModel& instance();

        void addConnectedClient(mqttbroker::broker::lib::Mqtt* mqtt, iot::mqtt::packets::Connect& connect);
        void delDisconnectedClient(mqttbroker::broker::lib::Mqtt* mqtt);

        const std::map<mqttbroker::broker::lib::Mqtt*, iot::mqtt::packets::Connect>& getConnectedClinets() const;

    protected:
        std::map<mqttbroker::broker::lib::Mqtt*, iot::mqtt::packets::Connect> connectedClients;
    };

} // namespace mqttbroker::broker::lib

#endif // MQTTBROKER_LIB_MQTTMODEL_H
