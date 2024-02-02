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
#include <Mandala/Mandala.h>

class TelemetryDB : public DatabaseSession
{
    Q_OBJECT
public:
    explicit TelemetryDB(QObject *parent, QString sessionName);

    void markCacheInvalid(quint64 telemetryID);
    QList<quint64> invalidCacheList();
    void clearInvalidCacheList();

    // fields UID mapping
    quint64 field_key(mandala::uid_t uid);
    quint64 field_key(QString name);
    mandala::uid_t mandala_uid(QString name);
    mandala::uid_t mandala_uid(quint64 field_key);

    typedef QMap<mandala::uid_t, quint64> FieldsByUID;
    typedef QMap<QString, quint64> FieldsByName;
    typedef QMap<QString, mandala::uid_t> UidByName;
    void updateFieldsMap(FieldsByUID byUID, FieldsByName byName);

    Fact *f_trash;
    Fact *f_stop;
    Fact *f_cache;
    Fact *f_stats;

private:
    QMutex pMutex; //property access mutex

    FieldsByUID _fieldsByUID;
    FieldsByName _fieldsByName;
    UidByName _uidByName;

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

protected:
    bool run(QSqlQuery &query);

private:
    Records records;

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

// check if telemetry is available in database and untrash it
class DBReqTelemetryRecover : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryRecover(QString hash)
        : DBReqTelemetry()
        , _hash(hash)
    {}

private:
    QString _hash;
    bool run(QSqlQuery &query);

signals:
    void available(quint64 telemetryID, QString hash);
    void unavailable(QString hash);
};
