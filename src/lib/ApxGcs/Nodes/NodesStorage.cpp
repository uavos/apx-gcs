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
#include "NodesStorage.h"
#include "NodeItem.h"
#include "Nodes.h"

#include <App/App.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
NodesStorage::NodesStorage(Nodes *nodes)
    : QObject(nodes)
    , loading(false)
    , nodes(nodes)
    , loadDicts(false)
{}
//=============================================================================
void NodesStorage::loadNodeInfo(NodeItem *node)
{
    //qDebug()<<"req info";
    DBReqNodesLoadInfo *req = new DBReqNodesLoadInfo(node->sn());
    connect(req,
            &DBReqNodesLoadInfo::infoLoaded,
            this,
            &NodesStorage::infoLoaded,
            Qt::QueuedConnection);
    req->exec();
}
void NodesStorage::saveNodeInfo(NodeItem *node)
{
    //qDebug()<<"save info";
    QVariantMap info;
    info.insert("sn", node->sn());
    info.insert("time", node->lastSeenTime);
    info.insert("name", node->title());
    info.insert("version", node->version());
    info.insert("hardware", node->hardware());
    DBReqNodesSaveInfo *req = new DBReqNodesSaveInfo(info);
    req->exec();
}
void NodesStorage::infoLoaded(QVariantMap info)
{
    //qDebug()<<"info loaded";
    NodeItem *node = nodes->node(info.value("sn").toString());
    if (!node)
        return;
    if (node->infoValid())
        return;
    //qDebug()<<"dbInfoLoaded"<<this<<record;
    node->setTitle(info.value("name").toString());
    node->setHardware(info.value("hardware").toString());
}
//=============================================================================
void NodesStorage::saveNodeUser(NodeItem *node)
{
    //qDebug()<<"save user";
    QVariantMap info;
    info.insert("machineUID", App::machineUID());
    info.insert("username", App::username());
    info.insert("hostname", App::hostname());
    DBReqNodesSaveUser *req = new DBReqNodesSaveUser(node->sn(), info);
    req->exec();
}
//=============================================================================
//=============================================================================
void NodesStorage::loadDictCache(NodeItem *node)
{
    DBReqNodesLoadDict *req = new DBReqNodesLoadDict(node->sn(), node->conf_hash);
    connect(req,
            &DBReqNodesLoadDict::dictInfoFound,
            node,
            &NodeItem::setDictInfo,
            Qt::QueuedConnection);
    connect(req,
            &DBReqNodesLoadDict::dictLoaded,
            this,
            &NodesStorage::dictLoaded,
            Qt::QueuedConnection);
    connect(req, &DatabaseRequest::finished, node, &NodeItem::requestDict, Qt::QueuedConnection);
    req->exec();
    //qDebug()<<"req cache";
}
//=============================================================================
void NodesStorage::saveDictCache(NodeItem *node, const DictNode::Dict &dict)
{
    //qDebug()<<"save cache";
    QVariantMap info;
    info.insert("sn", node->sn());
    info.insert("time", node->lastSeenTime);
    info.insert("chash", dict.chash);

    DBReqNodesSaveDict *req = new DBReqNodesSaveDict(info, dict);
    connect(req,
            &DBReqNodesSaveDict::dictInfoFound,
            node,
            &NodeItem::setDictInfo,
            Qt::QueuedConnection);
    req->exec();
}
//=============================================================================
void NodesStorage::dictLoaded(QVariantMap info, DictNode::Dict dict)
{
    //qDebug()<<"cache loaded";
    NodeItem *node = nodes->node(info.value("sn").toString());
    if (!node)
        return;
    if (node->dictValid())
        return;
    node->setDictInfo(info);
    node->dictReceived(dict);
}
//=============================================================================
//=============================================================================
void NodesStorage::loadNodeConfig(NodeItem *node, quint64 nconfID)
{
    if (!node->dictValid())
        return;
    //if(!dataValid())return;
    //Vehicles::instance()->vdb->nodeDataRead(this,nconfID);
    DBReqNodesLoadNconf *req = new DBReqNodesLoadNconf(nconfID);
    connect(req, &DBReqNodesLoadNconf::nconfFound, node, &NodeItem::setNconfID, Qt::QueuedConnection);
    connect(req,
            &DBReqNodesLoadNconf::configLoaded,
            this,
            &NodesStorage::configLoaded,
            Qt::QueuedConnection);
    loading = true;
    connect(
        req, &DatabaseRequest::finished, this, [this]() { loading = false; }, Qt::QueuedConnection);
    req->exec();
}
void NodesStorage::saveNodeConfig(NodeItem *node)
{
    if (!node->dataValid())
        return;
    if (node->reconf())
        return;
    if (node->dictInfo.isEmpty()) {
        qWarning() << "missing dictInfo";
        return;
    }

    QVariantMap values;
    foreach (const QString s, node->allFieldsByName.keys()) {
        NodeField *f = node->allFieldsByName.value(s);
        values.insert(s, f->toString());
    }
    qDebug() << "save node config" << node->title() << node->status();
    loading = false;
    DBReqNodesSaveNconf *req = new DBReqNodesSaveNconf(node->dictInfo, values, 0);
    connect(req, &DBReqNodesSaveNconf::nconfFound, node, &NodeItem::setNconfID, Qt::QueuedConnection);
    req->exec();
}
void NodesStorage::configLoaded(QVariantMap info, QVariantMap values)
{
    NodeItem *node = nodes->node(info.value("sn").toString());
    if (!node)
        return;
    node->loadConfigValues(values);
    apxMsg() << QString("%1 (%2): %3")
                    .arg(tr("Data restored"))
                    .arg(node->title())
                    .arg(backupTitle(info.value("time").toULongLong(),
                                     info.value("title").toString()));
}
//=============================================================================
void NodesStorage::restoreNodeConfig(NodeItem *node)
{
    if (!node->dictValid())
        return;
    DBReqNodesLoadNconfLatest *req = new DBReqNodesLoadNconfLatest(node->sn());
    connect(req, &DBReqNodesLoadNconf::nconfFound, node, &NodeItem::setNconfID, Qt::QueuedConnection);
    connect(req,
            &DBReqNodesLoadNconfLatest::configLoaded,
            this,
            &NodesStorage::configLoaded,
            Qt::QueuedConnection);
    loading = true;
    connect(
        req, &DatabaseRequest::finished, this, [this]() { loading = false; }, Qt::QueuedConnection);
    req->exec();
}
QString NodesStorage::backupTitle(quint64 time, QString title)
{
    QString s = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(time))
                    .toString("yyyy MMM dd hh:mm:ss");
    if (!title.isEmpty())
        s += QString(" (%1)").arg(title);
    return s;
}
//=============================================================================
//=============================================================================
void NodesStorage::saveConfiguration(bool force)
{
    QList<quint64> list;
    foreach (NodeItem *node, nodes->nodes()) {
        if (node->reconf() || (!node->nconfID) || (!node->dictValid()) || (!node->dataValid())
            || (force == false && node->conf_hash.isEmpty())) {
            qDebug() << "invalid nodes in configuration" << node->nconfID;
            list.clear();
            break;
        }
        if (node->offline() && force == false)
            continue;
        list.append(node->nconfID);
    }
    quint64 vehicleID = nodes->vehicle->dbKey;
    if (list.isEmpty() || (vehicleID == 0 && force == false)) {
        nodes->vehicle->message(tr("Configuration not saved"), AppNotify::Warning);
        return;
    }
    if (!vehicleID)
        vehicleID = Vehicles::instance()->f_local->dbKey;
    DBReqNodesSaveConfig *req = new DBReqNodesSaveConfig(list, vehicleID, "");
    connect(req,
            &DBReqNodesSaveConfig::configInfoFound,
            this,
            &NodesStorage::setConfigInfo,
            Qt::QueuedConnection);
    connect(req,
            &DBReqNodesSaveConfig::configUpdated,
            this,
            &NodesStorage::vehicleConfigUpdated,
            Qt::QueuedConnection);
    connect(req,
            &DBReqNodesSaveConfig::configCreated,
            this,
            &NodesStorage::vehicleConfigCreated,
            Qt::QueuedConnection);
    req->exec();
}
void NodesStorage::vehicleConfigUpdated()
{
    nodes->vehicle->message(tr("Configuration exists"), AppNotify::Important);
}
void NodesStorage::vehicleConfigCreated()
{
    nodes->vehicle->message(tr("Configuration created"), AppNotify::Important);
}
//=============================================================================
void NodesStorage::setConfigInfo(QVariantMap info)
{
    configInfo = info;
    emit configInfoUpdated();
}
//=============================================================================
//=============================================================================
void NodesStorage::loadConfiguration(QString hash)
{
    DBReqNodesLoadConfig *req = new DBReqNodesLoadConfig(hash);
    if (nodes->vehicle->isReplay()) {
        if ((!nodes->modified()) && nodes->nodesCount() > 0 && loadedHash == hash)
            return;
        loadedHash = hash;
    }
    connect(req,
            &DBReqNodesLoadConfig::loaded,
            this,
            &NodesStorage::loadedConfiguration,
            Qt::QueuedConnection);
    req->exec();
}
//=============================================================================
void NodesStorage::loadedConfiguration(QVariantMap configInfo, QList<QVariantMap> data)
{
    QString title = configInfo.value("title").toString();
    if (nodes->vehicle->isReplay()) {
        if (this->configInfo.value("vehicleID").toString()
            != configInfo.value("vehicleID").toString()) {
            nodes->clear();
        }
    } else {
        if (nodes->nodesCount() > 0) {
            int rcnt = importConfigs(data);
            QString s = tr("Config loaded");
            if (!title.isEmpty())
                s.append(QString(" (%1)").arg(title));
            if (data.size() != rcnt) {
                s.append(QString(" %1 of %2 nodes").arg(rcnt).arg(data.size()));
                nodes->vehicle->message(s, AppNotify::Warning);
            } else
                nodes->vehicle->message(s, AppNotify::Important);
            return;
        }
    }
    setConfigInfo(configInfo);
    //create missing nodes
    loading = true;
    for (int i = 0; i < data.size(); ++i) {
        newNodeDict(data.at(i).value("dictInfo").value<QVariantMap>(),
                    data.at(i).value("dict").value<DictNode::Dict>());
    }
    //load nodes data
    for (int i = 0; i < data.size(); ++i) {
        newNodeConfig(data.at(i).value("nconfID").toUInt(),
                      data.at(i).value("nconfInfo").value<QVariantMap>(),
                      data.at(i).value("values").value<QVariantMap>());
    }
    loading = false;
    QString s = tr("Configuration loaded");
    if (!title.isEmpty())
        s.append(QString(" (%1)").arg(title));
    nodes->vehicle->message(s, AppNotify::Important);
}
//=============================================================================
void NodesStorage::newNodeDict(QVariantMap info, DictNode::Dict dict)
{
    //qDebug()<<info;
    QString sn = info.value("sn").toString();
    NodeItem *n = nodes->node(sn);
    if (n) {
        if (n->dictInfo == info) {
            qWarning() << "node exists" << n->title();
            return;
        }
        qWarning() << "node exists but dict changed" << n->title();
        n->clear();
    } else {
        n = nodes->appendNode(sn, nullptr);
    }
    n->setTitle(info.value("name").toString());
    n->setVersion(info.value("version").toString());
    n->setHardware(info.value("hardware").toString());
    n->setInfoValid(true);
    dictLoaded(info, dict);
}
//=============================================================================
void NodesStorage::newNodeConfig(quint64 nconfID, QVariantMap info, QVariantMap values)
{
    //qDebug()<<info;
    QString sn = info.value("sn").toString();
    NodeItem *n = nodes->node(sn);
    if (!n) {
        qWarning() << "missing node" << sn;
        return;
    }
    n->loadConfigValues(values);
    n->setDataValid(true);
    n->setNconfID(nconfID);
}
//=============================================================================
int NodesStorage::importConfigs(QList<QVariantMap> data)
{
    int icnt = 0;
    QHash<NodeItem *, QString> snmap;
    QHash<NodeItem *, int> nmap;
    int priority = 0;

    for (int i = 0; i < data.size(); ++i) {
        const QVariantMap &info = data.at(i).value("nconfInfo").value<QVariantMap>();

        QString sn = info.value("sn").toString();
        QString node_name = info.value("name").toString();
        QString comment = info.value("title").toString();
        //qDebug()<<ssn<<node_name<<comment;
        NodeItem *node = nullptr;
        //match by serial numner
        priority = 0;
        node = nodes->node(sn);
        if (node) {
            nmap.insert(node, priority);
            //qDebug()<<"Node by sn: "<<node->name<<node_name<<comment;
        }
        //find matching node by name and comment
        priority++;
        if (!node) {
            if (comment.size()) {
                foreach (NodeItem *i, nodes->nodes())
                    if (i->title() == node_name && i->status() == comment) {
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
                foreach (NodeItem *i, nodes->nodes())
                    if (i->title() == node_name && i->status().isEmpty()) {
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
                foreach (NodeItem *i, nodes->nodes())
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
                foreach (NodeItem *i, nodes->nodes())
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
        NodeItem *node = snmap.key(sn, nullptr);
        if (node && node->loadConfigValues(values) > 0) {
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
//=============================================================================
//=============================================================================
//=============================================================================
