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

class UnitSaveInfo : public Request
{
    Q_OBJECT
public:
    explicit UnitSaveInfo(const QJsonObject &info)
        : _info(info)
        , _time(QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}

    bool run(QSqlQuery &query) override;

private:
    const QJsonObject _info;
    quint64 _unitID{};
    quint64 _time;

signals:
    void foundID(quint64 key);
};

class UnitSaveConf : public Request
{
    Q_OBJECT
public:
    explicit UnitSaveConf(QString uid,
                          QList<quint64> nodeConfIDs,
                          QString title,
                          QString notes = {},
                          quint64 time = 0)
        : _uid(uid)
        , _nodeConfIDs(nodeConfIDs)
        , _title(title)
        , _notes(notes)
        , _time(time ? time : static_cast<quint64>(QDateTime::currentDateTime().toMSecsSinceEpoch()))
    {}
    bool run(QSqlQuery &query);

private:
    QString _uid;
    QList<quint64> _nodeConfIDs;
    QString _title;
    QString _notes;
    quint64 _time;

signals:
    void confSaved(QString hash, QString title);
};

class UnitLoadConf : public Request
{
    Q_OBJECT
public:
    explicit UnitLoadConf(quint64 unitConfID)
        : _unitConfID(unitConfID)
    {}
    bool run(QSqlQuery &query);

    auto conf() const { return _conf; }

private:
    quint64 _unitConfID;
    QJsonObject _conf;

signals:
    void confLoaded(QJsonObject conf);
};

class UnitImportConf : public Request
{
    Q_OBJECT
public:
    explicit UnitImportConf(const QJsonObject &conf)
        : _conf(conf)
    {}
    bool run(QSqlQuery &query);

private:
    const QJsonObject _conf;
};

} // namespace nodes
} // namespace db
