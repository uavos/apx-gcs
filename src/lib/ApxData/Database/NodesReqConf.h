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

#include "NodesReq.h"

namespace db {
namespace nodes {

class NodeSaveConf : public Request
{
    Q_OBJECT
public:
    explicit NodeSaveConf(const quint64 dictID, const QJsonObject &values, quint64 time = 0)
        : _dictID(dictID)
        , _values(values)
        , _time(time ? time : QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}

    bool run(QSqlQuery &query);

    auto nodeConfID() const { return _nodeConfID; }

private:
    const quint64 _dictID;
    const QJsonObject _values;
    const quint64 _time;
    quint64 _nodeConfID{};

    quint64 _getValueID(QSqlQuery &query, const QJsonValue &v);

signals:
    void confSaved(quint64 confID);
};

class NodeLoadConf : public Request
{
    Q_OBJECT
public:
    explicit NodeLoadConf(const quint64 dictID, const QString &hash) // latest if no hash
        : _dictID(dictID)
        , _hash(hash)
    {}
    explicit NodeLoadConf(quint64 nodeConfID)
        : _nodeConfID(nodeConfID)
    {}

    bool run(QSqlQuery &query);

    const auto &values() const { return _values; }
    auto time() const { return _time; }

protected:
    const quint64 _dictID{};
    const QString _hash;
    quint64 _nodeConfID{};

    quint64 _time{};
    QJsonObject _values;

signals:
    void confLoaded(QJsonObject values);
};

} // namespace nodes
} // namespace db
