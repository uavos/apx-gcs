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
#include <QtCore>
#include <Database/DatabaseSession.h>
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

private:
    QMutex pMutex; //property access mutex
    TelemetryFieldsMap m_fieldsMap;
    QList<quint64> m_invalidCacheList;
    quint64 latestInvalidCacheID;
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
#endif
