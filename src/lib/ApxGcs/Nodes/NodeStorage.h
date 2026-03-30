/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <Database/DatabaseModel.h>
#include <Fact/Fact.h>

class NodeItem;

class NodeStorage : public Fact
{
    Q_OBJECT
public:
    explicit NodeStorage(NodeItem *node, Fact *parent);

    auto dbmodel() const { return _dbmodel; }
    auto confID() const { return _confID; }

private:
    QPointer<NodeItem> _node;
    DatabaseModel *_dbmodel;
    quint64 _dictID{};
    quint64 _confID{};

    QStringList get_names(Fact *f, QStringList path = QStringList());

private slots:
    void dictMetaLoaded(QJsonObject jso);

    void dbRequestRecordsList();
    void dbRequestRecordInfo(quint64 id);

public slots:
    void saveNodeInfo();
    void saveNodeDict();
    void saveNodeConf();

    void loadLatestNodeConf();
    void loadNodeConf(quint64 id);

    void updateConfID(quint64 confID);

    void loadNodeMeta();

signals:
    void confSaved();
};
