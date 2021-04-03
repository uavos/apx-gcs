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

#include "VehiclesDB.h"

class DBReqSaveVehicleInfo : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqSaveVehicleInfo(QVariantMap info)
        : DBReqVehicles()
        , _info(info)
        , _time(QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}

    bool run(QSqlQuery &query) override;

    quint64 vehicleID{};

private:
    QVariantMap _info;
    quint64 _time;

signals:
    void foundID(quint64 key);
};

class DBReqSaveVehicleConfig : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqSaveVehicleConfig(QString vuid,
                                    QList<quint64> nconfIDs,
                                    QString title,
                                    QString notes = QString(),
                                    quint64 time = 0)
        : DBReqVehicles()
        , _vuid(vuid)
        , _nconfIDs(nconfIDs)
        , _title(title)
        , _notes(notes)
        , _time(time ? time : static_cast<quint64>(QDateTime::currentDateTime().toMSecsSinceEpoch()))
    {}
    bool run(QSqlQuery &query);

private:
    QString _vuid;
    QList<quint64> _nconfIDs;
    QString _title;
    QString _notes;
    quint64 _time;

signals:
    void configSaved(QString hash, QString title);
};

class DBReqLoadVehicleConfig : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqLoadVehicleConfig(QString hash)
        : DBReqVehicles()
        , _hash(hash)
    {}
    bool run(QSqlQuery &query);

private:
    QString _hash;

signals:
    void configLoaded(QVariantMap config);
};

class DBReqImportVehicleConfig : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqImportVehicleConfig(QVariantMap config)
        : DBReqVehicles()
        , _config(config)
    {}
    bool run(QSqlQuery &query);

private:
    QVariantMap _config;
};
