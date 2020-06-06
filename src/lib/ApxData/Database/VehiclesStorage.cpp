/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "VehiclesStorage.h"
#include "VehiclesReqDict.h"
#include "VehiclesReqNconf.h"
#include "VehiclesReqVehicle.h"

#include <Protocols/ProtocolVehicle.h>

#include <App/App.h>

VehiclesStorage::VehiclesStorage(QObject *parent)
    : QObject(parent)
{}

void VehiclesStorage::saveVehicleInfo(ProtocolVehicle *vehicle)
{
    QVariantMap info;
    info.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    info.insert("uid", vehicle->uid());
    info.insert("callsign", vehicle->title());
    info.insert("squawk", vehicle->squawkText());
    DBReqSaveVehicleInfo *req = new DBReqSaveVehicleInfo(info);
    connect(
        req,
        &DBReqSaveVehicleInfo::foundID,
        this,
        [this](quint64 key) { m_vehicle_key = key; },
        Qt::QueuedConnection);
    req->exec();
}

void VehiclesStorage::loadNodeInfo(ProtocolNode *node)
{
    DBReqVehiclesLoadInfo *req = new DBReqVehiclesLoadInfo(node->sn());
    connect(
        req,
        &DBReqVehiclesLoadInfo::infoLoaded,
        node,
        [node](QVariantMap info) {
            if (!node)
                return;
            if (node->identValid())
                return;
            node->setTitle(info.value("name").toString());
            node->setHardware(info.value("hardware").toString());
        },
        Qt::QueuedConnection);
    req->exec();
}
void VehiclesStorage::saveNodeInfo(ProtocolNode *node)
{
    QVariantMap info;
    info.insert("sn", node->sn());
    info.insert("time", node->lastSeenTime());
    info.insert("name", node->title());
    info.insert("version", node->version());
    info.insert("hardware", node->hardware());
    DBReqVehiclesSaveInfo *req = new DBReqVehiclesSaveInfo(info);
    req->exec();
}
void VehiclesStorage::saveNodeUser(ProtocolNode *node)
{
    QVariantMap info;
    info.insert("machineUID", App::machineUID());
    info.insert("username", App::username());
    info.insert("hostname", App::hostname());
    DBReqVehiclesSaveUser *req = new DBReqVehiclesSaveUser(node->sn(), info);
    req->exec();
}

void VehiclesStorage::saveNodeDict(ProtocolNode *node, const ProtocolNode::Dict &dict)
{
    QVariantMap info;
    info.insert("sn", node->sn());
    info.insert("time", node->lastSeenTime());
    info.insert("hash", QString("%1").arg(node->ident().hash, 8, 16, QChar('0')).toUpper());

    DBReqVehiclesSaveDict *req = new DBReqVehiclesSaveDict(info, dict);
    /*connect(req,
            &DBReqVehiclesSaveDict::dictInfoFound,
            node,
            &NodeItem::setDictInfo,
            Qt::QueuedConnection);*/
    req->exec();
}
