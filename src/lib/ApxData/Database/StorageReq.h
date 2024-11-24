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

#include "TelemetryFileEvents.h"
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

    auto telemetryID() const { return _telemetryID; }

private:
    qint64 _t;
    QString _fileName;
    QJsonObject _info;
    bool _trash;

    //result
    quint64 _telemetryID{};

public:
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

public:
    virtual bool run(QSqlQuery &query);

signals:
    void fileOpened(QString filePath);
};

class TelemetryWriteRecordFields : public Request
{
    Q_OBJECT
public:
    explicit TelemetryWriteRecordFields(quint64 telemetryID, QJsonObject info, bool restore = false)
        : Request()
        , telemetryID(telemetryID)
        , info(info)
        , restore(restore)
    {}

private:
    quint64 telemetryID;
    QJsonObject info;
    bool restore;

public:
    bool run(QSqlQuery &query);
};

// export-import helpers

class TelemetryExport : public Request
{
    Q_OBJECT
public:
    explicit TelemetryExport(QString format, QString src, QString dst)
        : Request()
        , _format(format)
        , _src(src)
        , _dst(dst)
    {}

private:
    QString _format;
    QString _src;
    QString _dst;

protected:
    bool run(QSqlQuery &query);
signals:
    void progress(int v);
};

class TelemetryImport : public Request
{
    Q_OBJECT
public:
    explicit TelemetryImport(QString src)
        : Request()
        , _src(src)
    {}

private:
    QString _src;

protected:
    bool run(QSqlQuery &query);
signals:
    void progress(int v);
    void recordAvailable(quint64 telemetryID);
};

// DB maintenance helpers

class TelemetryStats : public Request
{
    Q_OBJECT
protected:
    bool run(QSqlQuery &query);
signals:
    void totals(quint64 total, quint64 trash, quint64 files);
};

class TelemetryEmptyTrash : public Request
{
    Q_OBJECT
protected:
    bool run(QSqlQuery &query);
signals:
    void progress(int v);
};

class TelemetrySyncFiles : public Request
{
    Q_OBJECT
public:
    static QString defaultBasename(const QJsonObject &info);

protected:
    bool run(QSqlQuery &query);
signals:
    void progress(int v);
};

} // namespace storage
} // namespace db
