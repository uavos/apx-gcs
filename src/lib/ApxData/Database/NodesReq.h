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

#include "NodesSession.h"

namespace db {
namespace nodes {

class RequestNode : public Request
{
    Q_OBJECT
public:
    explicit RequestNode(const QString &uid)
        : _uid(uid)
    {}
    explicit RequestNode(quint64 nodeID)
        : _nodeID(nodeID)
    {}

    virtual bool run(QSqlQuery &query) override;

protected:
    QString _uid;
    quint64 _nodeID{};

signals:
    void foundID(quint64 key);
};

class NodeSaveInfo : public RequestNode
{
    Q_OBJECT
public:
    explicit NodeSaveInfo(const QJsonObject &info)
        : RequestNode(info["uid"].toString())
        , _info(info)
    {}

    bool run(QSqlQuery &query) override;

private:
    const QJsonObject _info;
};

class NodeLoadInfo : public RequestNode
{
    Q_OBJECT
public:
    explicit NodeLoadInfo(const QString &uid)
        : RequestNode(uid)
    {}
    explicit NodeLoadInfo(quint64 nodeID)
        : RequestNode(nodeID)
    {}

    bool run(QSqlQuery &query) override;

    const auto &info() const { return _info; }

private:
    QJsonObject _info;

signals:
    void infoLoaded(QJsonObject info);
};

} // namespace nodes
} // namespace db
