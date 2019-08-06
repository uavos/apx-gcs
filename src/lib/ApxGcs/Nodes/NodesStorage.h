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
#ifndef NodesStorage_H
#define NodesStorage_H
//=============================================================================
#include <Database/NodesReqDict.h>
#include <Database/NodesReqNconf.h>
#include <Database/NodesReqVehicle.h>
#include <Dictionary/DictNode.h>
#include <Protocols/ProtocolServiceNode.h>
#include <QtCore>
class Nodes;
class NodeItem;
//=============================================================================
class NodesStorage : public QObject
{
    Q_OBJECT

public:
    explicit NodesStorage(Nodes *nodes);

    QVariantMap configInfo;

    bool loading;

private:
    Nodes *nodes;
    QString loadedHash;
    bool loadDicts;

    static QString backupTitle(quint64 time, QString title);

    void newNodeDict(QVariantMap info, DictNode::Dict dict);
    void newNodeConfig(quint64 nconfID, QVariantMap info, QVariantMap values);
    int importConfigs(QList<QVariantMap> data);

public slots:
    void loadNodeInfo(NodeItem *node);
    void saveNodeInfo(NodeItem *node);
    void infoLoaded(QVariantMap info);

    void saveNodeUser(NodeItem *node);

    void loadDictCache(NodeItem *node);
    void saveDictCache(NodeItem *node, const DictNode::Dict &dict);
    void dictLoaded(QVariantMap info, DictNode::Dict dict);

    void loadNodeConfig(NodeItem *node, quint64 nconfID);
    void saveNodeConfig(NodeItem *node);
    void configLoaded(QVariantMap info, QVariantMap values);

    void restoreNodeConfig(NodeItem *node);

    void loadConfiguration(QString hash);
    void saveConfiguration(bool force = false);

private slots:
    //Configs
    void setConfigInfo(QVariantMap info);
    void loadedConfiguration(QVariantMap configInfo, QList<QVariantMap> data);
    void vehicleConfigUpdated();
    void vehicleConfigCreated();

signals:
    void configInfoUpdated();
};
//=============================================================================
#endif
