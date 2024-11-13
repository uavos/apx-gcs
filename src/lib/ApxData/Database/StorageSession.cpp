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
#include "TelemetryFileReader.h"

#include <App/AppDirs.h>

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
    connect(f_stats, &Fact::triggered, this, &Session::getStats);
    connect(this, &Fact::triggered, f_stats, &Fact::trigger);

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
                           "trash INTEGER",    // not null if record deleted
                           "sync INTEGER",     // not null if record was ever synced with file
                           "src INTEGER",      // source of record: 0=record, i=import, 2=share
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
bool TelemetryStats::run(QSqlQuery &query)
{
    query.prepare("SELECT COUNT(*) FROM Telemetry");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 cntTotal = query.value(0).toULongLong();

    query.prepare("SELECT COUNT(*) FROM Telemetry WHERE trash IS NOT NULL");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 cntTrash = query.value(0).toULongLong();

    // count files
    auto fi = QFileInfo(Session::telemetryFilePath("*"));
    auto files = fi.absoluteDir().entryList({fi.fileName()}, QDir::Files);
    quint64 cntFiles = files.size();

    // report stats
    apxMsg() << tr("Telemetry records").append(":") << cntTotal << tr("total").append(",")
             << cntTrash << tr("trash");

    if (cntFiles != cntTotal)
        apxMsgW() << tr("Telemetry files").append(":") << cntFiles << tr("of") << cntTotal;

    emit totals(cntTotal, cntTrash, cntFiles);
    return true;
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
bool TelemetryEmptyTrash::run(QSqlQuery &query)
{
    db->disable();
    emit progress(0);
    bool rv = false;
    do {
        query.prepare("SELECT COUNT(*) FROM Telemetry WHERE trash IS NOT NULL");
        if (!query.exec())
            break;
        if (!query.next())
            break;

        qint64 cnt = query.value(0).toLongLong();
        qint64 dcnt = 0;
        if (cnt > 0) {
            apxMsg() << tr("Permanently deleting %1 records").arg(cnt).append("...");

            while (dcnt < cnt) {
                query.prepare("SELECT key,file FROM Telemetry WHERE trash IS NOT NULL ORDER BY "
                              "time DESC LIMIT 1");
                if (!query.exec())
                    break;
                if (!query.next())
                    break;
                quint64 key = query.value(0).toULongLong();
                auto file = query.value("file").toString();

                if (!db->transaction(query))
                    break;
                query.prepare("DELETE FROM Telemetry WHERE key=?");
                query.addBindValue(key);
                if (!query.exec())
                    break;
                if (!db->commit(query))
                    break;

                // remove file
                auto filePath = Session::telemetryFilePath(file);
                qDebug() << "Removing file" << filePath;
                if (!QFile::moveToTrash(filePath)) {
                    apxMsgW() << tr("Failed to remove file") << filePath;
                }

                dcnt++;
                emit progress((dcnt * 100 / cnt));
                if (discarded())
                    break;
            }
        }
        if (dcnt < cnt)
            apxMsgW() << tr("Telemetry trash not empty") << cnt - dcnt;
        else {
            apxMsg() << tr("Telemetry trash is empty");
            rv = true;
        }
    } while (0);
    emit progress(-1);
    db->enable();
    return rv;
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
bool TelemetrySyncFiles::run(QSqlQuery &query)
{
    db->disable();
    emit progress(0);

    bool rv = false;
    do {
        // remove sync flag from db
        query.prepare("UPDATE Telemetry SET sync=NULL");
        if (!query.exec()) {
            apxMsgW() << tr("Failed to clear sync flags");
            break;
        }

        // fix file names
        auto fi = QFileInfo(Session::telemetryFilePath("*"));
        auto filesTotal = fi.absoluteDir().count();
        auto filesCount = 0;
        QDirIterator it(fi.absolutePath(), QDir::Files);
        while (it.hasNext()) {
            emit progress(filesCount++ * 100 / filesTotal);

            auto fi = QFileInfo(it.next());
            qDebug() << fi.fileName();

            TelemetryFileReader reader(fi.absoluteFilePath());
            if (!reader.open()) {
                apxConsoleW() << tr("File error").append(':') << fi.fileName();
                // QFile::moveToTrash(fi.absoluteFilePath());
                continue;
            }
            // check file name
            auto timestamp = QDateTime::fromMSecsSinceEpoch(reader.timestamp());
            timestamp.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(reader.utc_offset()));
            auto unitName = reader.info()["unit"]["name"].toString();
            if (unitName.isEmpty()) {
                auto info = Session::telemetryFilenameParse(fi.absoluteFilePath());
                unitName = info["unitName"].toString();
            }

            auto basename = Session::telemetryFileBasename(timestamp, unitName);
            auto filePath = Session::telemetryFilePath(basename);
            if (fi.absoluteFilePath() != filePath) {
                apxConsole() << tr("Rename %1 to %2")
                                    .arg(fi.fileName(), QFileInfo(filePath).fileName());
                // check if same file exists
                /*if (QFile::exists(filePath)) {
                    qWarning() << "File exists" << filePath;
                    auto hash = reader.get_hash();
                    auto hash2 = TelemetryFileReader(filePath).get_hash();
                    if (hash == hash2) {
                        qDebug() << "Removing duplicate" << fi.fileName();
                        QFile::remove(filePath);
                    } else {
                        // rename with some name suffix
                        fixedFileName
                            = TelemetryFileWriter::prepare_file_name(time,
                                                                     unitName,
                                                                     fi.dir().absolutePath());
                        newPath = fi.dir().absoluteFilePath(fixedFileName);
                        ok = rename(newPath);
                        if (!ok) {
                            qWarning()
                                << "Failed to rename" << fi.fileName() << "to" << fixedFileName;
                        }
                    }
                }*/
                /*if (!reader.rename(fi.absoluteFilePath(), filePath)) {
                apxConsoleW() << tr("Failed to rename").append(':') << fi.fileName();
                continue;
            }*/
            }
            basename = fi.completeBaseName();

            // find file in db
            query.prepare("SELECT * FROM Telemetry WHERE file=?");
            query.addBindValue(basename);
            if (!query.exec())
                break;
            if (!query.next()) {
                qWarning() << "File not found in db" << basename;
            }
        }

        // success
        rv = true;
    } while (0);
    emit progress(-1);
    db->enable();
    return rv;
}

/*


bool TelemetryFileReader::fix_name()
{
    // fix rename if necessary
    auto time = QDateTime::fromMSecsSinceEpoch(timestamp(), QTimeZone(utc_offset()));
    auto unitName = info()["unit"]["name"].toString();
    auto fixedFileName = TelemetryFileWriter::prepare_file_name(time, unitName);
    auto fi = QFileInfo(fileName());
    if (fixedFileName != fi.fileName()) {
        auto newPath = fi.dir().absoluteFilePath(fixedFileName);
        // rename file
        close();

        bool ok = rename(newPath);
        if (!ok) {
            qWarning() << "Failed to rename" << fi.fileName() << "to" << fixedFileName;
            if (QFile::exists(newPath)) {
                qWarning() << "File" << newPath << "already exists";
                // compare two files and remove the old one if equal
                auto hash = get_hash();
                auto newHash = TelemetryFileReader(newPath).get_hash();
                if (hash == newHash) {
                    qDebug() << "Removing duplicate" << fi.fileName();
                    QFile::remove(newPath);
                    ok = rename(newPath);
                } else {
                    // rename with some name suffix
                    fixedFileName = TelemetryFileWriter::prepare_file_name(time,
                                                                           unitName,
                                                                           fi.dir().absolutePath());
                    newPath = fi.dir().absoluteFilePath(fixedFileName);
                    ok = rename(newPath);
                    if (!ok) {
                        qWarning() << "Failed to rename" << fi.fileName() << "to" << fixedFileName;
                    }
                }
            }
        }
        if (ok) {
            qDebug() << "Renamed" << fi.fileName() << "to" << fixedFileName;
        }

        if (!open(newPath))
            return false;
    }
    return true;
}

*/
