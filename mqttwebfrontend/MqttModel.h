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

#ifndef APPS_MQTTBROKER_MQTTWETFRONTEND_MQTTMODEL_H
#define APPS_MQTTBROKER_MQTTWETFRONTEND_MQTTMODEL_H

#include "SocketContext.h"
#include "iot/mqtt/packets/Connect.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker::webfrontend {

    class MqttModel {
    private:
        MqttModel();

    public:
        static MqttModel& instance();

        void addConnectedClient(apps::mqttbroker::webfrontend::SocketContext* socketContext, const iot::mqtt::packets::Connect& connect);
        void delDisconnectedClient(apps::mqttbroker::webfrontend::SocketContext* socketContext);

        const std::map<apps::mqttbroker::webfrontend::SocketContext*, iot::mqtt::packets::Connect>& getConnectedClinets();

    protected:
        std::map<apps::mqttbroker::webfrontend::SocketContext*, iot::mqtt::packets::Connect> connectedClients;
    };

} // namespace apps::mqttbroker::webfrontend

#endif // APPS_MQTTBROKER_MQTTWETFRONTEND_MQTTMODEL_H
