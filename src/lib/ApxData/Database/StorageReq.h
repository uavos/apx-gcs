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

#include "DatabaseModel.h"
#include "StorageSession.h"

#include "TelemetryFileReader.h"
#include "TelemetryFileWriter.h"

namespace db {
namespace storage {

class TelemetryCreateRecord : public Request
{
    Q_OBJECT
public:
    explicit TelemetryCreateRecord(qint64 t, QString fileName, QJsonObject info, bool trash)
        : Request()
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

class TelemetryModelRecordsList : public Request
{
    Q_OBJECT
public:
    explicit TelemetryModelRecordsList(QString filter)
        : Request()
        , _filter(filter)
    {}

protected:
    QString _filter;
    bool run(QSqlQuery &query);

signals:
    void recordsList(DatabaseModel::RecordsList records);
};

class TelemetryLoadInfo : public Request
{
    Q_OBJECT
public:
    explicit TelemetryLoadInfo(quint64 id)
        : Request()
        , _id(id)
    {}

protected:
    quint64 _id;
    bool run(QSqlQuery &query);

signals:
    void modelInfo(quint64 id, QJsonObject modelInfo);
    void recordInfo(quint64 id, QJsonObject info, QString notes);
};

class TelemetryModelTrash : public Request
{
    Q_OBJECT
public:
    explicit TelemetryModelTrash(quint64 id, bool trash = false)
        : Request()
        , _id(id)
        , _trash(trash)
    {}

protected:
    quint64 _id;
    bool _trash;
    virtual bool run(QSqlQuery &query);
};

class TelemetryLoadFile : public TelemetryLoadInfo
{
    Q_OBJECT
public:
    explicit TelemetryLoadFile(quint64 id)
        : TelemetryLoadInfo(id)
    {
        connect(this, &Request::discardRequested, &_reader, &TelemetryFileReader::abort);
    }

    auto reader() const { return &_reader; }

protected:
    TelemetryFileReader _reader;

    virtual bool run(QSqlQuery &query);
};

class TelemetryWriteInfo : public Request
{
    Q_OBJECT
public:
    explicit TelemetryWriteInfo(quint64 telemetryID, QVariantMap info, bool restore = false)
        : Request()
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

} // namespace storage
} // namespace db
