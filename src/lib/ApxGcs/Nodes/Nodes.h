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
#pragma once

#include "NodeItem.h"

#include <Protocols/ProtocolNodes.h>
#include <Protocols/ProtocolViewBase.h>

class Vehicle;

class Nodes : public ProtocolViewBase<ProtocolNodes>
{
    Q_OBJECT

    Q_PROPERTY(int nodesCount READ nodesCount NOTIFY nodesCountChanged)

public:
    explicit Nodes(Vehicle *vehicle, ProtocolNodes *protocol);

    Vehicle *vehicle;

    Fact *f_upload;
    Fact *f_search;
    Fact *f_stop;
    Fact *f_reload;
    Fact *f_clear;
    Fact *f_status;

    //LookupConfigs *f_lookup;
    //NodesStorage *storage;
    //NodesShare *f_share;
    //Fact *f_save;

    NodeItem *node(const QString &sn) { return m_sn_map.value(sn, nullptr); }
    QList<NodeItem *> nodes() { return m_sn_map.values(); }

    NodeItem *add(ProtocolNode *protocol);

    void syncLater(int timeout = 2000);

    void loadConfValue(const QString &sn, QString s);

private:
    QMap<QString, NodeItem *> m_sn_map;
    QList<NodeItemBase *> m_groups;

    //sync
    QDateTime m_syncTimestamp;
    QTimer m_syncTimer;
    QElapsedTimer m_syncRequestTime;
    int m_syncCount{0};

    bool check_valid() const;

private slots:
    void updateStatus();
    void updateActions();

    void sync();

    void search();
    void clear();
    void reload();
    void upload();
    void stop();

    void dataExchangeFinished();
    void nodeFound(ProtocolNode *protocol);

signals:
    void syncDone();

public slots:
    void save();

    //---------------------------------------
    // PROPERTIES
public:
    int nodesCount() const;

protected:
signals:
    void nodesCountChanged();
};
