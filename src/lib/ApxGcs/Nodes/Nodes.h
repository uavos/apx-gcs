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
#ifndef Nodes_H
#define Nodes_H
//=============================================================================
#include "LookupConfigs.h"
#include "NodeItem.h"
#include "NodesShare.h"
#include "NodesStorage.h"
#include <Fact/Fact.h>
#include <Protocols/ProtocolService.h>
#include <QDomDocument>
#include <QtCore>
class Vehicle;
typedef QList<NodeItem *> NodesList;
//=============================================================================
class Nodes : public NodeItemBase
{
    Q_OBJECT

    Q_PROPERTY(int nodesCount READ nodesCount NOTIFY nodesCountChanged)

public:
    explicit Nodes(Vehicle *parent);

    Vehicle *vehicle;

    Fact *f_upload;
    Fact *f_request;
    Fact *f_stop;
    Fact *f_reload;
    Fact *f_clear;
    Fact *f_nstat;

    LookupConfigs *f_lookup;

    NodesStorage *storage;
    NodesShare *f_share;

    Fact *f_save;

    NodeItem *node(const QString &sn) { return snMap.value(sn, nullptr); }
    QList<NodeItem *> nodes() { return snMap.values(); }

    NodeItem *appendNode(const QString &sn, ProtocolServiceNode *protocol);
    void removeNode(const QString &sn);

    void syncLater(int timeout = 2000, bool enforce = true);
    void loadConfValue(const QString &sn, QString s);

    void uploadedSync();
    void nconfSavedSync();

    QDateTime syncTimestamp;

    QList<NodeItemBase *> nGroups;
    QList<QString> skipCache;

protected:
    //override
    virtual QVariant data(int col, int role) const;

private:
    QMap<QString, NodeItem *> snMap;

    //sync
    QTimer syncTimer;
    int syncCount;
    bool syncActive;
    bool syncUpdate;
    bool syncUpload;
    QElapsedTimer syncTime;

private slots:
    void updateStatus();
    void updateActions();
    void syncFinished();
    void sync();

    //upgrade
    void upgradeStarted(QString sn);
    void upgradeFinished(QString sn, bool success);

public slots:
    void request();
    void clear();
    void reload();
    void upload();
    void stop();

    void nstat();
    void rebootAll();

    void save();
    void clearCache();

    //---------------------------------------
    // PROPERTIES
public:
    int nodesCount() const;

protected:
    int m_nodesCount;

signals:
    void nodesCountChanged();
};
//=============================================================================
#endif
