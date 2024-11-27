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

#include <Database/DatabaseModel.h>
#include <Database/TelemetryDB.h>
#include <Fact/Fact.h>

#include "TelemetryFileReader.h"
#include "TelemetryFileWriter.h"

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

class DBReqTelemetryCreateRecord : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryCreateRecord(qint64 t, QString fileName, QJsonObject info, bool trash)
        : DBReqTelemetry()
        , _t(t)
        , _fileName(fileName)
        , _info(info)
        , _trash(trash)
    {}

private:
    qint64 _t;
    QString _fileName;
    QJsonObject _info;
    bool _trash;

    //result
    quint64 _telemetryID{};

protected:
    bool run(QSqlQuery &query);

signals:
    void recordCreated(quint64 telemetryID);
};

class DBReqTelemetryModelRecordsList : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryModelRecordsList(QString filter)
        : DBReqTelemetry()
        , _filter(filter)
    {}

protected:
    QString _filter;
    bool run(QSqlQuery &query);

signals:
    void recordsList(DatabaseModel::RecordsList records);
};

class DBReqTelemetryLoadInfo : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryLoadInfo(quint64 id)
        : DBReqTelemetry()
        , _id(id)
    {}

protected:
    quint64 _id;
    bool run(QSqlQuery &query);

signals:
    void modelInfo(quint64 id, QJsonObject modelInfo);
    void recordInfo(quint64 id, QJsonObject info, QString notes);
};

class DBReqTelemetryModelTrash : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryModelTrash(quint64 id, bool trash = false)
        : DBReqTelemetry()
        , _id(id)
        , _trash(trash)
    {}

protected:
    quint64 _id;
    bool _trash;
    virtual bool run(QSqlQuery &query);
};

class DBReqTelemetryLoadFile : public DBReqTelemetryLoadInfo
{
    Q_OBJECT
public:
    explicit DBReqTelemetryLoadFile(quint64 id)
        : DBReqTelemetryLoadInfo(id)
    {
        connect(this, &DBReqTelemetry::discardRequested, &_reader, &TelemetryFileReader::abort);
    }

    auto reader() const { return &_reader; }

protected:
    TelemetryFileReader _reader;

    virtual bool run(QSqlQuery &query);
};