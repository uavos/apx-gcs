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
#include <QtCore>

class DBReqTelemetryFindCache : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryFindCache(quint64 telemetryID)
        : DBReqTelemetry()
        , cacheID(0)
        , telemetryID(telemetryID)
        , t(QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}
    //result
    quint64 cacheID;

protected:
    quint64 telemetryID;
    qint64 t;
    virtual bool run(QSqlQuery &query);
signals:
    void cacheFound(quint64 telemetryID);
    void cacheNotFound(quint64 telemetryID);
};

class DBReqTelemetryMakeCache : public DBReqTelemetryFindCache
{
    Q_OBJECT
public:
    explicit DBReqTelemetryMakeCache(quint64 telemetryID, bool forceUpdate)
        : DBReqTelemetryFindCache(telemetryID)
        , newCacheID(0)
        , forceUpdate(forceUpdate)
    {}
    //result
    quint64 newCacheID;

protected:
    bool forceUpdate;
    bool run(QSqlQuery &query);
};

class DBReqTelemetryMakeStats : public DBReqTelemetryMakeCache
{ //will make cache and stats
    Q_OBJECT
public:
    explicit DBReqTelemetryMakeStats(quint64 telemetryID)
        : DBReqTelemetryMakeCache(telemetryID, false)
    {}
    //result
    QVariantMap stats;

protected:
    bool run(QSqlQuery &query);
signals:
    void statsFound(quint64 telemetryID, QVariantMap stats);
    void statsUpdated(quint64 telemetryID, QVariantMap stats);
};

class DBReqTelemetryReadData : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryReadData(quint64 telemetryID)
        : DBReqTelemetry()
        , cacheID(0)
        , telemetryID(telemetryID)
    {}
    //result
    quint64 cacheID;
    QVariantMap info;
    Records records;
    QMap<quint64, QString> fieldNames;

protected:
    quint64 telemetryID;

    virtual bool run(QSqlQuery &query);
signals:
    void progress(quint64 telemetryID, int v);
    void dataLoaded(quint64 telemetryID,
                    quint64 cacheID,
                    DatabaseRequest::Records records,
                    QMap<quint64, QString> fieldNames);
};

//Player
class DBReqTelemetryReadEvents : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryReadEvents(quint64 cacheID, quint64 time)
        : DBReqTelemetry()
        , cacheID(cacheID)
        , time(time)
    {}

protected:
    bool run(QSqlQuery &query);

private:
    quint64 cacheID;
    quint64 time;

signals:
    void eventsLoaded(DatabaseRequest::Records records);
};

class DBReqTelemetryReadEvent : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryReadEvent(quint64 cacheID, quint64 time, QString name)
        : DBReqTelemetry()
        , cacheID(cacheID)
        , time(time)
        , name(name)
    {}

protected:
    bool run(QSqlQuery &query);

private:
    quint64 cacheID;
    quint64 time;
    QString name;

signals:
    void eventLoaded(QString value, QString uid, bool uplink);
};

class DBReqTelemetryReadConfData : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryReadConfData(quint64 cacheID, quint64 time)
        : DBReqTelemetry()
        , cacheID(cacheID)
        , time(time)
    {}

protected:
    bool run(QSqlQuery &query);

private:
    quint64 cacheID;
    quint64 time;

signals:
    void confLoaded(DatabaseRequest::Records records);
};

//share
class DBReqTelemetryReadSharedHashId : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryReadSharedHashId(QString hash)
        : DBReqTelemetry()
        , telemetryID(0)
        , hash(hash)
    {}
    explicit DBReqTelemetryReadSharedHashId(quint64 telemetryID)
        : DBReqTelemetry()
        , telemetryID(telemetryID)
    {}
    //result
    quint64 telemetryID;
    QString hash;

protected:
    bool run(QSqlQuery &query);
signals:
    void foundID(quint64 telemetryID);
    void foundHash(QString hash);
};

class DBReqTelemetryFindDataHash : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryFindDataHash(QString hash)
        : DBReqTelemetry()
        , telemetryID(0)
        , hash(hash)
    {}
    //result
    quint64 telemetryID;
    QString hash;

protected:
    bool run(QSqlQuery &query);
signals:
    void foundID(quint64 telemetryID);
};

class DBReqTelemetryDeleteRecord : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryDeleteRecord(quint64 telemetryID)
        : DBReqTelemetry()
        , telemetryID(telemetryID)
    {}

protected:
    bool run(QSqlQuery &query);

private:
    quint64 telemetryID;

signals:
    void deletedID(quint64 telemetryID);
};
