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
#include "TelemetryFilesWorker.h"

#include <App/App.h>

TelemetryFilesWorker::TelemetryFilesWorker(QObject *parent)
    : QThread(parent)
{
    connect(App::instance(), &App::loadingFinished, this, [this]() {
        QTimer::singleShot(3000, this, [this]() { start(LowPriority); });
    });
}

TelemetryFilesWorker::~TelemetryFilesWorker()
{
    requestAbort();
    wait();

    for (auto i : queue)
        delete i;
}

bool TelemetryFilesWorker::event(QEvent *event)
{
    if (event->type() == QEvent::DeferredDelete && isRunning()) {
        // We have been asked to shut down later but were blocked,
        // so the owning QFileSystemModel proceeded with its shut-down
        // and deferred the destruction of the gatherer.
        // If we are still blocked now, then we have three bad options:
        // terminate, wait forever (preventing the process from shutting down),
        // or accept a memory leak.
        requestAbort();
        if (!wait(5000)) {
            // If the application is shutting down, then we terminate.
            // Otherwise assume that sooner or later the thread will finish,
            // and we delete it then.
            if (QCoreApplication::closingDown())
                terminate();
            else
                connect(this, &QThread::finished, this, [this] { delete this; });
            return true;
        }
    }

    return QThread::event(event);
}

void TelemetryFilesWorker::requestAbort()
{
    requestInterruption();
    QMutexLocker locker(&mutex);
    condition.wakeAll();
}

void TelemetryFilesWorker::add(TelemetryFilesJob *job)
{
    QMutexLocker locker(&mutex);
    queue.enqueue(job);
    condition.wakeAll();
}

void TelemetryFilesWorker::run()
{
    forever {
        if (isInterruptionRequested())
            return;

        // Disallow termination while we are holding a mutex or can be
        // woken up cleanly.
        setTerminationEnabled(false);
        QMutexLocker locker(&mutex);
        while (!isInterruptionRequested() && queue.isEmpty())
            condition.wait(&mutex);
        if (isInterruptionRequested())
            return;

        auto job = queue.dequeue();
        locker.unlock();
        // Some of the system APIs we call when gathering file infomration
        // might hang (e.g. waiting for network), so we explicitly allow
        // termination now.
        setTerminationEnabled(true);

        job->run();
        emit job->finished();
        delete job;
    }
}

void TelemetryFilesJobList::run()
{
    QStringList filters;
    filters << QStringList() << QString("*.").append(telemetry::APXTLM_FTYPE);

    QStringList files;

    QDirIterator it(_path, filters, QDir::Files);
    while (!isInterruptionRequested() && it.hasNext()) {
        it.next();
        auto name = it.fileName().section('.', 0, -2);
        if (name.isEmpty())
            continue;
        files.append(name);
    }

    files.sort(Qt::CaseInsensitive);
    std::reverse(files.begin(), files.end());

    if (isInterruptionRequested())
        return;

    emit result(files);
}

void TelemetryFilesJobInfo::run()
{
    if (!reader.open(_path))
        return;

    if (isInterruptionRequested())
        return;

    // fix rename if necessary
    auto timestamp = QDateTime::fromMSecsSinceEpoch(reader.timestamp(),
                                                    QTimeZone(reader.utc_offset()));
    auto callsign = reader.callsign();
    auto fileName = TelemetryFileWriter::prepare_file_name(timestamp, callsign);
    auto fi = QFileInfo(_path);
    if (fileName != fi.fileName()) {
        auto newPath = fi.dir().absoluteFilePath(fileName);
        if (!QFile::rename(_path, newPath)) {
            qWarning() << "Failed to rename" << fi.fileName() << "to" << fileName;
        } else {
            qDebug() << "Renamed" << fi.fileName() << "to" << fileName;
            _path = newPath;
            reader.close();
            if (!reader.open(_path))
                return;
        }
    }

    // auto parse payloads for small files
    if (!reader.is_parsed() && reader.size() < 1024 * 1024) {
        reader.parse_payload();
    }

    emit result(reader.info(), _id);
}

void TelemetryFilesJobParse::run()
{
    if (!reader.open(_path))
        return;

    if (!reader.parse_payload())
        return;

    emit result(reader.info(), _id);
}
