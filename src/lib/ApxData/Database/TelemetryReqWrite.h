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
#ifndef TelemetryReqWrite_H
#define TelemetryReqWrite_H
//=============================================================================
#include "TelemetryDB.h"
#include <QtCore>
//=============================================================================
class DBReqTelemetryNewRecord : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryNewRecord(
        QString vehicleUID, QString callsign, QString comment, bool recording, qint64 t)
        : DBReqTelemetry()
        , telemetryID(0)
        , vehicleUID(vehicleUID)
        , callsign(callsign)
        , comment(comment)
        , recording(recording)
        , t(t)
    {}
    //result
    quint64 telemetryID;

private:
    QString vehicleUID;
    QString callsign;
    QString comment;
    bool recording;
    qint64 t;

protected:
    bool run(QSqlQuery &query);
signals:
    void idUpdated(quint64 telemetryID);
};
//=============================================================================
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
//=============================================================================
class DBReqTelemetryWriteData : public DBReqTelemetryWriteBase
{
    Q_OBJECT
public:
    explicit DBReqTelemetryWriteData(
        quint64 telemetryID, quint64 t, quint64 fieldID, double value, bool uplink)
        : DBReqTelemetryWriteBase(telemetryID, t, uplink)
        , fieldID(fieldID)
        , value(value)
    {}

private:
    quint64 fieldID;
    double value;

protected:
    bool run(QSqlQuery &query);
};
//=============================================================================
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
//=============================================================================
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
//=============================================================================
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
//=============================================================================
//=============================================================================
#endif
