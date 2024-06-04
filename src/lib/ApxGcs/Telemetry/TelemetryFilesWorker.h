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

#include <QtCore>

#include "TelemetryFileFormat.h"
#include "TelemetryFileReader.h"
#include "TelemetryFileWriter.h"

class TelemetryFilesJob;

class TelemetryFilesWorker : public QThread
{
    Q_OBJECT

public:
    explicit TelemetryFilesWorker(QObject *parent = nullptr);
    ~TelemetryFilesWorker();

    void requestAbort();

    void add(TelemetryFilesJob *job);

protected:
    bool event(QEvent *event) override;

private:
    void run() override;

    mutable QMutex mutex;
    QWaitCondition condition;
    QQueue<TelemetryFilesJob *> queue;
};

class TelemetryFilesJob : public QObject
{
    Q_OBJECT
signals:
    void finished();

public:
    explicit TelemetryFilesJob(TelemetryFilesWorker *worker, QString path)
        : QObject(worker)
        , _worker(worker)
        , _path(path)
    {}
    virtual ~TelemetryFilesJob() = default;

    void schedule() { _worker->add(this); }

    virtual void run() = 0;

protected:
    TelemetryFilesWorker *_worker;
    QString _path;

    bool isInterruptionRequested() const { return _worker->isInterruptionRequested(); }
};

class TelemetryFilesJobList : public TelemetryFilesJob
{
    Q_OBJECT
public:
    explicit TelemetryFilesJobList(TelemetryFilesWorker *worker, QString path)
        : TelemetryFilesJob(worker, path)
    {}
    void run() override;

signals:
    void result(QStringList files);
};

class TelemetryFilesJobInfo : public TelemetryFilesJob
{
    Q_OBJECT
public:
    explicit TelemetryFilesJobInfo(TelemetryFilesWorker *worker, QString path, int id)
        : TelemetryFilesJob(worker, path)
        , _id(id)
    {}
    virtual void run() override;

    TelemetryFileReader reader; // used to connect to signels

protected:
    int _id;

signals:
    void result(QVariantMap info, int id);
};

class TelemetryFilesJobParse : public TelemetryFilesJobInfo
{
    Q_OBJECT
public:
    explicit TelemetryFilesJobParse(TelemetryFilesWorker *worker, QString path, int id)
        : TelemetryFilesJobInfo(worker, path, id)
    {}
    void run() override;
};
