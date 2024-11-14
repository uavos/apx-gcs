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

#include "StorageSession.h"
#include "Database.h"

#include "StorageReq.h"

using namespace db::storage;

Request::Request()
    : DatabaseRequest(Database::instance()->storage)
{}

Session::Session(QObject *parent, QString sessionName)
    : DatabaseSession(parent, "storage", sessionName, {}, AppDirs::storage())
{
    f_sync
        = new Fact(this, "sync", tr("Sync files"), tr("Sync files with database"), NoFlags, "sync");
    connect(f_sync, &Fact::triggered, this, &Session::syncFiles);
    connect(f_sync, &Fact::progressChanged, this, [this]() { setProgress(f_sync->progress()); });

    f_stats = new Fact(this, "stats", tr("Get statistics"), tr("Analyze totals"), NoFlags, "numeric");
    connect(this, &Fact::triggered, f_stats, &Fact::trigger);
    connect(f_stats, &Fact::triggered, this, &Session::getStats);

    f_trash = new Fact(this,
                       "trash",
                       tr("Empty trash"),
                       tr("Permanently remove deleted records"),
                       Remove);
    connect(f_trash, &Fact::triggered, this, &Session::emptyTrash);
    connect(f_trash, &Fact::progressChanged, this, [this]() { setProgress(f_trash->progress()); });

    f_stop = new Fact(this, "stop", tr("Abort"), tr("Abort current operation"), Action | Stop);
    f_stop->setEnabled(false);
    connect(this, &Fact::progressChanged, this, [this]() { f_stop->setEnabled(progress() >= 0); });

    // create tables

    new DBReqMakeTable(this,
                       "Telemetry",
                       {
                           "key INTEGER PRIMARY KEY NOT NULL",

                           // fields written on file creation
                           "time INTEGER", // [ms since epoch] file creation time
                           "file TEXT",    // file name with data (no ext)

                           // identity info
                           "unitUID TEXT",  // unit UID from linked table
                           "unitName TEXT", // unit name
                           "unitType TEXT", // unit type (GCU, UAV, etc)
                           "confName TEXT", // conf name or comment

                           // info extracted from file
                           "size INTEGER",     // [bytes] file size
                           "hash TEXT",        // file content hash
                           "info TEXT",        // JSON info meta data
                           "duration INTEGER", // [ms] total time of telemetry data

                           // local record status and flags
                           "trash INTEGER", // not null if record deleted
                           "sync INTEGER", // sync status: NULL=unsynced, 1=synced file, 2=synced info
                           "src INTEGER",      // source of record: NULL=record, 1=import, 2=share
                           "imported INTEGER", // [ms since epoch] file import time
                           "parsed INTEGER",   // [ms since epoch] file parsing time
                           "notes TEXT",       // user notes or comments if any
                       });

    new DBReqMakeIndex(this, "Telemetry", "time", false);
    new DBReqMakeIndex(this, "Telemetry", "unitUID", false);
    new DBReqMakeIndex(this, "Telemetry", "unitName", false);
    new DBReqMakeIndex(this, "Telemetry", "file", true);
    new DBReqMakeIndex(this, "Telemetry", "trash", false);
}

QString Session::telemetryFileBasename(QDateTime timestamp, QString unitName)
{
    QString basename;
    {
        QStringList st;

        // add human readable timestamp
        auto ts = timestamp.toString("yyMMdd_HHmm_sszzz");
        auto utc_offset = timestamp.offsetFromUtc();
        ts.append(utc_offset > 0 ? 'E' : 'W');
        auto offset_mins = std::abs(utc_offset) / 60;
        ts.append(QString::number(offset_mins % 60 ? offset_mins : offset_mins / 60));
        st.append(ts);

        basename = st.join('_').toUpper();
    }

    {
        // fix unitName style
        auto cs = unitName.trimmed().toUpper().toUtf8();
        QString cs_fixed;
        for (auto s = cs.data(); *s; ++s) {
            auto c = *s;
            if (c >= '0' && c <= '9') { // allow any numbers
                cs_fixed.append(c);
            } else if (c >= 'A' && c <= 'Z') { // allow capital letters
                cs_fixed.append(c);
                continue;
            }
        }
        basename.append('-').append(cs_fixed.isEmpty() ? "U" : cs_fixed);
    }

    return basename;
}
QString Session::telemetryFilePath(const QString &basename)
{
    auto filePath = AppDirs::storage().absoluteFilePath(telemetry::APXTLM_FTYPE);
    filePath = QDir(filePath).absoluteFilePath(basename + '.' + telemetry::APXTLM_FTYPE);
    return filePath;
}
QString Session::telemetryFilePathUnique(const QString &basename)
{
    // find name (append-01, -02, etc if necessary)
    for (int i = 0; i < 100; ++i) {
        auto s = basename;
        if (i > 0)
            s.append(QString("-%1").arg(i, 2, 10, QChar('0')));

        auto fpath = db::storage::Session::telemetryFilePath(s);
        if (!QFile::exists(fpath))
            return fpath;
    }
    return {};
}

QJsonObject Session::telemetryFilenameParse(const QString &filePath)
{
    QJsonObject info;

    auto name = QFileInfo(filePath).completeBaseName();
    info["name"] = name;
    info["path"] = QFileInfo(filePath).absoluteFilePath();

    auto ts_s = name.section('-', 0, 0);
    auto ts_parts = ts_s.split('_');
    if (ts_parts.size() != 3) {
        qWarning() << "invalid file name" << filePath;
        return {};
    }

    // parse timestamp
    auto ts_fmt = QString("yyMMdd_HHmm_sszzz");
    auto s = ts_s.left(ts_fmt.size());
    auto timestamp = QDateTime::fromString(s, ts_fmt);
    if (!timestamp.isValid()) {
        qWarning() << "invalid timestamp" << s;
        return {};
    }

    s = ts_s.mid(ts_fmt.size());
    auto utc_offset_sign = s.startsWith('E') ? 1 : s.startsWith('W') ? -1 : 0;
    if (utc_offset_sign != 0) {
        auto offset_mins = s.mid(1).toInt();
        if (offset_mins <= 12)
            offset_mins *= 60;
        timestamp.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(offset_mins * 60 * utc_offset_sign));
    } else {
        qWarning() << "invalid utc offset sign" << s;
    }
    info["timestamp"] = timestamp.toMSecsSinceEpoch();
    info["utc_offset"] = timestamp.offsetFromUtc();

    auto cs = name.section('-', 1, 1);
    if (cs.isEmpty()) {
        qWarning() << "invalid unitName" << name;
    } else {
        info["unitName"] = cs.section('-', 0, 0);
    }

    return info;
}

void Session::getStats()
{
    auto req = new TelemetryStats();
    connect(
        req,
        &TelemetryStats::totals,
        this,
        [this](quint64 total, quint64 trash, quint64 files) {
            f_stats->setValue(
                QString("%1 %2, %3 %4").arg(total).arg(tr("total")).arg(trash).arg(tr("trash")));
            f_trash->setEnabled(trash > 0);
            f_sync->setValue(total == files ? tr("Synced") : QString("%1/%2").arg(files).arg(total));
        },
        Qt::QueuedConnection);
    req->exec();
}

void Session::emptyTrash()
{
    auto req = new TelemetryEmptyTrash();
    connect(
        req,
        &TelemetryEmptyTrash::progress,
        this,
        [this](int v) { f_trash->setProgress(v); },
        Qt::QueuedConnection);
    connect(req, &TelemetryEmptyTrash::finished, this, &Session::getStats, Qt::QueuedConnection);
    connect(f_stop, &Fact::triggered, req, &DatabaseRequest::discard, Qt::QueuedConnection);
    req->exec();
}

void Session::syncFiles()
{
    auto req = new TelemetrySyncFiles();
    connect(
        req,
        &TelemetrySyncFiles::progress,
        this,
        [this](int v) { f_sync->setProgress(v); },
        Qt::QueuedConnection);
    connect(req, &TelemetrySyncFiles::finished, this, &Session::getStats, Qt::QueuedConnection);
    connect(f_stop, &Fact::triggered, req, &DatabaseRequest::discard, Qt::QueuedConnection);
    req->exec();
}
