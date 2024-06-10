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

class DBReqTelemetryRecordInfo : public DBReqTelemetry
{
    Q_OBJECT
public:
    explicit DBReqTelemetryRecordInfo(quint64 id)
        : DBReqTelemetry()
        , _id(id)
    {}

protected:
    quint64 _id;
    bool run(QSqlQuery &query);

signals:
    void recordModelInfo(quint64 id, QJsonObject info);
    void recordInfo(quint64 id, QJsonObject info);
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

class DBReqTelemetryLoadFile : public DBReqTelemetryRecordInfo
{
    Q_OBJECT
public:
    explicit DBReqTelemetryLoadFile(quint64 id)
        : DBReqTelemetryRecordInfo(id)
    {
        connect(this, &DBReqTelemetry::discardRequested, &_reader, &TelemetryFileReader::abort);
    }

    auto reader() const { return &_reader; }

protected:
    TelemetryFileReader _reader;

    virtual bool run(QSqlQuery &query);
};
