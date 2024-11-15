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

#include "TelemetryDB.h"
#include <Protocols/PBase.h>

class DBReqTelemetryNewRecord : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryNewRecord(QString vehicleUID,
                                     QString callsign,
                                     QString comment,
                                     bool recording,
                                     qint64 t,
                                     QString fileName)
        : DBReqTelemetry()
        , telemetryID(0)
        , vehicleUID(vehicleUID)
        , callsign(callsign)
        , comment(comment)
        , recording(recording)
        , t(t)
        , fileName(fileName)
    {}
    //result
    quint64 telemetryID;

private:
    QString vehicleUID;
    QString callsign;
    QString comment;
    bool recording;
    qint64 t;
    QString fileName;

protected:
    bool run(QSqlQuery &query);
signals:
    void idUpdated(quint64 telemetryID);
};

class DBReqTelemetryWriteBase : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryWriteBase(quint64 telemetryID, quint64 t, bool uplink)
        : DBReqTelemetry()
        , telemetryID(telemetryID)
        , t(t)
        , uplink(uplink)
    {}
    quint64 telemetryID;
    quint64 t;

protected:
    bool uplink;
};

class DBReqTelemetryWriteData : public DBReqTelemetryWriteBase
{
    Q_OBJECT
public:
    explicit DBReqTelemetryWriteData(quint64 telemetryID,
                                     quint64 t,
                                     PBase::Values values,
                                     bool uplink)
        : DBReqTelemetryWriteBase(telemetryID, t, uplink)
        , _values(values)
    {}

private:
    PBase::Values _values;

protected:
    bool run(QSqlQuery &query);
};

class DBReqTelemetryWriteEvent : public DBReqTelemetryWriteBase
{
    Q_OBJECT
public:
    explicit DBReqTelemetryWriteEvent(quint64 telemetryID,
                                      quint64 t,
                                      const QString &name,
                                      const QString &value,
                                      const QString &uid,
                                      bool uplink)
        : DBReqTelemetryWriteBase(telemetryID, t, uplink)
        , name(name)
        , value(value)
        , uid(uid)
    {}

private:
    QString name;
    QString value;
    QString uid;

protected:
    bool run(QSqlQuery &query);
};

class DBReqTelemetryWriteInfo : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryWriteInfo(quint64 telemetryID, QVariantMap info, bool restore = false)
        : DBReqTelemetry()
        , telemetryID(telemetryID)
        , info(info)
        , restore(restore)
    {}

private:
    quint64 telemetryID;
    QVariantMap info;
    bool restore;

protected:
    bool run(QSqlQuery &query);
};

class DBReqTelemetryWriteSharedInfo : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryWriteSharedInfo(quint64 telemetryID, QVariantMap info)
        : DBReqTelemetry()
        , telemetryID(telemetryID)
        , info(info)
    {}

private:
    quint64 telemetryID;
    QVariantMap info;

protected:
    bool run(QSqlQuery &query);
};
