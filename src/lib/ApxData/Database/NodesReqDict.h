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

class NodeSaveDict : public RequestNode
{
    Q_OBJECT
public:
    explicit NodeSaveDict(const QString &uid, const QJsonObject &dict)
        : RequestNode(uid)
        , _dict(dict)
    {}

    bool run(QSqlQuery &query);
    auto dictID() const { return _dictID; }

private:
    const QJsonObject _dict;
    quint64 _dictID{};

signals:
    void dictSaved(quint64 dictID);
};

class NodeLoadDict : public RequestNode
{
    Q_OBJECT
public:
    // dict cache
    explicit NodeLoadDict(const QString &uid, const QString &cache_hash)
        : RequestNode(uid)
        , _cache_hash(cache_hash)
    {}

    // dict by id
    explicit NodeLoadDict(quint64 dictID)
        : RequestNode({})
        , _dictID(dictID)
    {}

    bool run(QSqlQuery &query);
    const auto &dict() const { return _dict; }

private:
    QString _cache_hash;
    quint64 _dictID{};

    QJsonObject _dict;

signals:
    void dictLoaded(quint64 dictID, QJsonObject dict);
    void dictMissing(QString cache_hash);
};

} // namespace nodes
} // namespace db
