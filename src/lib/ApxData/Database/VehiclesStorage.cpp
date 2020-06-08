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

VehiclesStorage::VehiclesStorage(ProtocolVehicle *vehicle, QObject *parent)
    : QObject(parent)
    , _vehicle(vehicle)
{}

void VehiclesStorage::saveVehicleInfo()
{
    QVariantMap info;
    info.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    info.insert("uid", _vehicle->uid());
    info.insert("callsign", _vehicle->title());
    info.insert("squawk", _vehicle->squawkText());
    DBReqSaveVehicleInfo *req = new DBReqSaveVehicleInfo(info);
    connect(req,
            &DBReqSaveVehicleInfo::foundID,
            _vehicle,
            &ProtocolVehicle::dbKeyFound,
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
    connect(req,
            &DBReqVehiclesSaveDict::dictInfoFound,
            node,
            &ProtocolNode::dbDictInfoFound,
            Qt::QueuedConnection);
    req->exec();
}
void VehiclesStorage::saveNodeConfig(ProtocolNode *node, QVariantMap values)
{
    if (!node->valid())
        return;
    if (node->ident().flags.bits.reconf)
        return;
    if (node->dbDictInfo().isEmpty()) {
        qWarning() << "missing dictInfo";
        return;
    }
    qDebug() << "save node config" << node->title() << node->value().toString();

    DBReqVehiclesSaveNconf *req = new DBReqVehiclesSaveNconf(node->dbDictInfo(), values, 0);
    connect(req,
            &DBReqVehiclesSaveNconf::nconfFound,
            node,
            &ProtocolNode::dbConfigIDFound,
            Qt::QueuedConnection);
    req->exec();
}

void VehiclesStorage::saveConfiguration(bool force)
{
    QList<quint64> list;
    for (auto i : _vehicle->nodes->nodes()) {
        if (!i->valid() || !i->dbConfigID() || i->dbDictInfo().isEmpty()) {
            qDebug() << "invalid nodes in configuration" << i->title();
            list.clear();
            break;
        }
        if (!i->enabled() && force == false)
            continue;
        list.append(i->dbConfigID());
    }
    quint64 vehicleID = _vehicle->dbKey();
    if (list.isEmpty() || (vehicleID == 0 && force == false)) {
        AppNotify::instance()->report(tr("Configuration not saved"),
                                      AppNotify::Warning,
                                      _vehicle->title());
        return;
    }
    if (!vehicleID)
        vehicleID = _vehicle->vehicles->local->dbKey();

    DBReqVehiclesSaveConfig *req = new DBReqVehiclesSaveConfig(list, vehicleID, "");
    connect(req,
            &DBReqVehiclesSaveConfig::configInfoFound,
            _vehicle,
            &ProtocolVehicle::dbConfigInfoFound,
            Qt::QueuedConnection);
    connect(
        req,
        &DBReqVehiclesSaveConfig::configUpdated,
        this,
        [this]() {
            AppNotify::instance()->report(tr("Configuration exists"),
                                          AppNotify::Important,
                                          _vehicle->title());
        },
        Qt::QueuedConnection);
    connect(
        req,
        &DBReqVehiclesSaveConfig::configCreated,
        this,
        [this]() {
            AppNotify::instance()->report(tr("Configuration created"),
                                          AppNotify::Important,
                                          _vehicle->title());
        },
        Qt::QueuedConnection);
    req->exec();
}

void VehiclesStorage::loadConfiguration(QString hash)
{
    DBReqVehiclesLoadConfig *req = new DBReqVehiclesLoadConfig(hash);
    if (_vehicle->isReplay()) {
        if ((!_vehicle->nodes->modified()) && _vehicle->nodes->nodes().size() > 0
            && _vehicle->dbConfigHash() == hash)
            return;
        _vehicle->dbSetConfigHash(hash);
    }
    connect(req,
            &DBReqVehiclesLoadConfig::loaded,
            this,
            &VehiclesStorage::loadedConfiguration,
            Qt::QueuedConnection);
    req->exec();
}
void VehiclesStorage::loadedConfiguration(QVariantMap configInfo, QList<QVariantMap> data)
{
    int loadedCnt = data.size();
    QString title = configInfo.value("title").toString();
    if (!_vehicle->isReplay()) {
        loadedCnt = 0; //importConfigs(data);
    } else {
        // vehicle is REPLAY
        _vehicle->dbConfigInfoFound(configInfo);

        for (int i = 0; i < data.size(); ++i) {
            const QVariantMap &info = data.at(i).value("dictInfo").value<QVariantMap>();
            const ProtocolNode::Dict &dict = data.at(i).value("dict").value<ProtocolNode::Dict>();

            QString sn = info.value("sn").toString();
            ProtocolNode *node = _vehicle->nodes->getNode(sn);
            do {
                if (node->identValid()) {
                    if (node->valid() && node->dbDictInfo() == info) {
                        qWarning() << "node exists" << node->title();
                        break;
                    }
                    qWarning() << "node exists but dict changed" << node->title();
                } else {
                    qWarning() << "new node" << info.value("name").toString();
                }
                node->setTitle(info.value("name").toString());
                node->setVersion(info.value("version").toString());
                node->setHardware(info.value("hardware").toString());
                node->setIdentValid(true);
                node->setDictValid(true);
                node->dbDictInfoFound(info);
                node->dictReceived(dict);
            } while (0);

            quint64 nconfID = data.at(i).value("nconfID").toULongLong();
            //const QVariantMap &nconfInfo = data.at(i).value("nconfInfo").value<QVariantMap>();
            const QVariantMap &valuesMap = data.at(i).value("values").value<QVariantMap>();
            QVariantList values;
            for (auto const &f : dict) {
                if (f.type < xbus::node::conf::type_field)
                    continue;
                values.append(valuesMap.value(f.name));
            }
            node->setValid(true);
            node->dbConfigIDFound(nconfID);
            node->confReceived(values);
        }
    }

    QString s = tr("Configuration loaded");
    if (!title.isEmpty())
        s.append(QString(" (%1)").arg(title));
    if (data.size() != loadedCnt) {
        s.append(QString(" %1 of %2 nodes").arg(loadedCnt).arg(data.size()));
        AppNotify::instance()->report(s, AppNotify::Warning, _vehicle->title());
    } else
        AppNotify::instance()->report(s, AppNotify::Important, _vehicle->title());
}
