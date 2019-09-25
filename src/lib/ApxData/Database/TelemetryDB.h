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
#ifndef TelemetrysDB_H
#define TelemetrysDB_H
//=============================================================================
#include <Database/DatabaseSession.h>
#include <QtCore>
//=============================================================================
class TelemetryDB : public DatabaseSession
{
    Q_OBJECT
public:
    explicit TelemetryDB(QObject *parent, QString sessionName);

    typedef QMap<quint64, QString> TelemetryFieldsMap;
    TelemetryFieldsMap fieldsMap();
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

    bool checkActive();

public slots:
    void emptyTrash();
    void emptyCache();
    void getStats();

signals:
    void invalidateRecords(); //called after record del
};
//=============================================================================
class DBReqTelemetry : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqTelemetry();

protected:
    virtual bool run(QSqlQuery &query);
};
//=============================================================================
class DBReqTelemetryUpdateFields : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryUpdateFields(Records records)
        : DBReqTelemetry()
        , records(records)
    {}

private:
    Records records;

protected:
    bool run(QSqlQuery &query);
signals:
    void countLoaded(quint64 count, QStringList titles);
};
//=============================================================================
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
//=============================================================================
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
//=============================================================================
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
//=============================================================================
#endif
