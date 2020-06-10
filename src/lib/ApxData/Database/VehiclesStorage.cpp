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

VehiclesStorage::VehiclesStorage(ProtocolVehicle *vehicle)
    : QObject(vehicle)
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
            if (!node->identValid()) {
                node->setTitle(info.value("name").toString());
                node->setHardware(info.value("hardware").toString());
            }
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
    info.insert("hash", node->identHash());

    DBReqVehiclesSaveDict *req = new DBReqVehiclesSaveDict(info, dict);
    connect(req,
            &DBReqVehiclesSaveDict::dictInfoFound,
            node,
            &ProtocolNode::dbDictInfoFound,
            Qt::QueuedConnection);
    req->exec();
}
void VehiclesStorage::loadNodeDict(ProtocolNode *node)
{
    DBReqVehiclesLoadDict *req = new DBReqVehiclesLoadDict(node->sn(), node->identHash());
    connect(req,
            &DBReqVehiclesLoadDict::dictInfoFound,
            node,
            &ProtocolNode::dbDictInfoFound,
            Qt::QueuedConnection);
    connect(
        req,
        &DBReqVehiclesLoadDict::dictLoaded,
        node,
        [node](QVariantMap info, const ProtocolNode::Dict &dict) {
            node->dbDictInfoFound(info);
            node->setDict(dict);
        },
        Qt::QueuedConnection);
    connect(req, &DatabaseRequest::finished, node, &ProtocolNode::requestDict, Qt::QueuedConnection);
    req->exec();
}

void VehiclesStorage::saveNodeConfig(ProtocolNode *node)
{
    if (!node->valid())
        return;
    if (node->ident().flags.bits.reconf)
        return;
    if (node->dbDictInfo().isEmpty()) {
        qWarning() << "missing dictInfo";
        return;
    }
    if (node->values().isEmpty()) {
        qWarning() << "missing values";
        return;
    }
    qDebug() << "save node config" << node->title() << node->value().toString();

    DBReqVehiclesSaveNconf *req = new DBReqVehiclesSaveNconf(node->dbDictInfo(), node->values(), 0);
    connect(req,
            &DBReqVehiclesSaveNconf::nconfFound,
            node,
            &ProtocolNode::dbConfigIDFound,
            Qt::QueuedConnection);
    req->exec();
}
void VehiclesStorage::loadNodeConfig(ProtocolNode *node, quint64 key)
{
    if (!node->valid())
        return;

    DBReqVehiclesLoadNconf *req = key ? new DBReqVehiclesLoadNconf(key)
                                      : new DBReqVehiclesLoadNconfLatest(node->sn());
    connect(req,
            &DBReqVehiclesLoadNconf::nconfFound,
            node,
            &ProtocolNode::dbConfigIDFound,
            Qt::QueuedConnection);
    connect(
        req,
        &DBReqVehiclesLoadNconf::configLoaded,
        node,
        [node](QVariantMap info, QVariantMap values) {
            node->confReceived(values);
            apxMsg() << QString("%1 (%2): %3")
                            .arg(tr("Data restored"))
                            .arg(node->title())
                            .arg(backupTitle(info.value("time").toULongLong(),
                                             info.value("title").toString()));
        },
        Qt::QueuedConnection);

    req->exec();
}
QString VehiclesStorage::backupTitle(quint64 time, QString title)
{
    QString s = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(time))
                    .toString("yyyy MMM dd hh:mm:ss");
    if (!title.isEmpty())
        s += QString(" (%1)").arg(title);
    return s;
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
            && _vehicle->dbConfigHash() == hash) {
            qDebug() << "vehicle config already loaded";
            return;
        }
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
    size_t loadedCnt = data.size();
    QString title = configInfo.value("title").toString();
    if (!_vehicle->isReplay()) {
        loadedCnt = importConfiguration(data);
    } else {
        // vehicle is REPLAY
        _vehicle->dbConfigInfoFound(configInfo);

        for (int i = 0; i < data.size(); ++i) {
            const QVariantMap &info = data.at(i).value("dictInfo").value<QVariantMap>();
            const ProtocolNode::Dict &dict = data.at(i).value("dict").value<ProtocolNode::Dict>();

            QString sn = info.value("sn").toString();
            ProtocolNode *node = _vehicle->nodes->getNode(sn);
            do {
                if (node->valid()) {
                    if (node->dbDictInfo().value("hash") == info.value("hash")) {
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
                node->dbDictInfoFound(info);
                node->setDict(dict);
            } while (0);

            quint64 nconfID = data.at(i).value("nconfID").toULongLong();
            //const QVariantMap &nconfInfo = data.at(i).value("nconfInfo").value<QVariantMap>();
            const QVariantMap &valuesMap = data.at(i).value("values").value<QVariantMap>();
            node->confReceived(valuesMap);
            node->setValid(true);
            node->dbConfigIDFound(nconfID);
        }
    }

    QString s = tr("Configuration loaded");
    if (!title.isEmpty())
        s.append(QString(" (%1)").arg(title));
    if (static_cast<size_t>(data.size()) != loadedCnt) {
        s.append(QString(" %1 of %2 nodes").arg(loadedCnt).arg(data.size()));
        AppNotify::instance()->report(s, AppNotify::Warning, _vehicle->title());
    } else
        AppNotify::instance()->report(s, AppNotify::Important, _vehicle->title());
}
size_t VehiclesStorage::importConfiguration(QList<QVariantMap> data)
{
    size_t icnt = 0;
    QHash<ProtocolNode *, QString> snmap;
    QHash<ProtocolNode *, int> nmap;
    int priority = 0;

    for (int i = 0; i < data.size(); ++i) {
        const QVariantMap &info = data.at(i).value("nconfInfo").value<QVariantMap>();

        QString sn = info.value("sn").toString();
        QString node_name = info.value("name").toString();
        QString comment = info.value("title").toString();
        //qDebug()<<ssn<<node_name<<comment;
        ProtocolNode *node = nullptr;
        //match by serial numner
        priority = 0;
        node = _vehicle->nodes->getNode(sn, false);
        if (node) {
            nmap.insert(node, priority);
            //qDebug()<<"Node by sn: "<<node->name<<node_name<<comment;
        }
        //find matching node by name and comment
        priority++;
        if (!node) {
            if (comment.size()) {
                for (auto i : _vehicle->nodes->nodes())
                    if (i->title() == node_name && i->value().toString() == comment) {
                        if (nmap.contains(i) && nmap.value(i) <= priority)
                            continue;
                        node = i;
                        nmap.insert(node, priority);
                        //qDebug()<<"Node by name+comment: "<<node->name<<node_name<<comment;
                        break;
                    }
            }
        } //else qDebug()<<"Node by sn: "<<node->name<<node_name<<comment;
        //find matching node by name without comment
        priority++;
        if (!node) {
            if (node_name != "servo") {
                for (auto i : _vehicle->nodes->nodes())
                    if (i->title() == node_name && i->value().toString().isEmpty()) {
                        if (nmap.contains(i) && nmap.value(i) <= priority)
                            continue;
                        node = i;
                        nmap.insert(node, priority);
                        //qDebug()<<"Node by name-no-comment: "<<node->name<<node_name<<comment;
                        break;
                    }
            }
        }
        //find matching node by name with any comment
        priority++;
        if (!node) {
            if (node_name != "servo") {
                for (auto i : _vehicle->nodes->nodes())
                    if (i->title() == node_name) {
                        if (nmap.contains(i) && nmap.value(i) <= priority)
                            continue;
                        node = i;
                        nmap.insert(node, priority);
                        //qDebug()<<"Node by name: "<<node->name<<node_name<<comment;
                        break;
                    }
            }
        }
        //find matching node by name part
        priority++;
        if (!node) {
            if (node_name.contains('.')) {
                node_name.remove(0, node_name.indexOf('.'));
                for (auto i : _vehicle->nodes->nodes())
                    if (i->title().endsWith(node_name)) {
                        if (nmap.contains(i) && nmap.value(i) <= priority)
                            continue;
                        node = i;
                        nmap.insert(node, priority);
                        //qDebug()<<"Node by name part: "<<node->name<<node_name<<comment;
                        break;
                    }
            }
        }
        if (!node)
            continue; //no matching nodes
        snmap.insert(node, sn);
        icnt++;
    }
    int rcnt = 0;
    QVariantList ignoredNconfs;
    for (int i = 0; i < data.size(); ++i) {
        const QVariantMap &info = data.at(i).value("nconfInfo").value<QVariantMap>();
        const QVariantMap &values = data.at(i).value("values").value<QVariantMap>();
        QString sn = info.value("sn").toString();
        auto node = snmap.key(sn, nullptr);
        if (node) {
            node->confReceived(values);
            rcnt++;
            continue;
        }
        ignoredNconfs.append(QVariant::fromValue(info));
    }
    for (int i = 0; i < ignoredNconfs.size(); ++i) {
        const QVariantMap &info = ignoredNconfs.at(i).value<QVariantMap>();
        apxMsgW() << tr("Ignored config") << info.value("name").toString()
                  << info.value("title").toString();
    }
    return rcnt;
}
