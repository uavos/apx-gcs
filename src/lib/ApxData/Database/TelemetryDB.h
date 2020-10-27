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

#include <Database/DatabaseSession.h>
#include <QtCore>

class TelemetryDB : public DatabaseSession
{
    Q_OBJECT
public:
    explicit TelemetryDB(QObject *parent, QString sessionName);

    typedef QMap<quint64, QString> TelemetryFieldsMap;
    typedef QMap<QString, QString> TelemetryFieldsAliases;

    TelemetryFieldsMap &fieldsMap();
    void setFieldsMap(const TelemetryFieldsMap &v);

    void markCacheInvalid(quint64 telemetryID);
    QList<quint64> invalidCacheList();
    void clearInvalidCacheList();

    Fact *f_trash;
    Fact *f_stop;
    Fact *f_cache;
    Fact *f_stats;

private:
    QMutex pMutex; //property access mutex

    TelemetryFieldsMap m_fieldsMap;

    QList<quint64> m_invalidCacheList;
    quint64 latestInvalidCacheID;

public slots:
    void emptyTrash();
    void emptyCache();
    void getStats();

signals:
    void invalidateRecords(); //called after record del
};

class DBReqTelemetry : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqTelemetry();

protected:
    virtual bool run(QSqlQuery &query);
};

class DBReqTelemetryUpdateMandala : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryUpdateMandala(Records records)
        : DBReqTelemetry()
        , records(records)
    {}

private:
    Records records;

protected:
    bool run(QSqlQuery &query);
signals:
    void progress(int v);
};

class DBReqTelemetryEmptyTrash : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryEmptyTrash()
        : DBReqTelemetry()
    {}

protected:
    bool run(QSqlQuery &query);
signals:
    void progress(int v);
};

class DBReqTelemetryEmptyCache : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryEmptyCache()
        : DBReqTelemetry()
    {}

protected:
    bool run(QSqlQuery &query);
signals:
    void progress(int v);
};

class DBReqTelemetryStats : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryStats()
        : DBReqTelemetry()
    {}

protected:
    bool run(QSqlQuery &query);
signals:
    void totals(quint64 total, quint64 trash);
};
