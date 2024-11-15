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
#include "StorageReq.h"

using namespace db::storage;

bool TelemetryCreateRecord::run(QSqlQuery &query)
{
    query.prepare("INSERT INTO Telemetry(time,file,unitUID,unitName,unitType,confName,trash,sync) "
                  "VALUES(?,?,?,?,?,?,?,?)");
    const auto &info = _info;
    query.addBindValue(_t);
    query.addBindValue(_fileName);
    query.addBindValue(info["unit"]["uid"].toVariant());
    query.addBindValue(info["unit"]["name"].toVariant());
    query.addBindValue(info["unit"]["type"].toVariant());
    query.addBindValue(info["conf"].toVariant());
    query.addBindValue(_trash ? 1 : QVariant());
    query.addBindValue(1); // sync status - file synced
    if (!query.exec())
        return false;
    _telemetryID = query.lastInsertId().toULongLong();

    emit recordCreated(_telemetryID);
    emit dbModified();
    //qDebug() << telemetryID;
    return true;
}

bool TelemetryModelRecordsList::run(QSqlQuery &query)
{
    QString s = "SELECT key, time FROM Telemetry";
    if (!_filter.isEmpty())
        s += " WHERE " + _filter;
    s += " ORDER BY time DESC";

    query.prepare(s);
    if (!query.exec())
        return false;

    DatabaseModel::RecordsList records;

    while (query.next()) {
        records.append(query.value(0).toULongLong());
    }

    emit recordsList(records);

    return true;
}

bool TelemetryLoadInfo::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM Telemetry WHERE key=?");
    query.addBindValue(_id);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    auto r = query.record();

    // prepare info for models
    QJsonObject j;

    QString stime = QDateTime::fromMSecsSinceEpoch(r.value("time").toLongLong())
                        .toString("yyyy MMM dd hh:mm:ss");
    QString unitName = r.value("unitName").toString();
    QString unitType = r.value("unitType").toString();
    QString confName = r.value("confName").toString();
    QString notes = r.value("notes").toString();
    QString total;
    quint64 t = r.value("duration").toULongLong();
    if (t > 0)
        total = AppRoot::timeToString(t / 1000);
    QStringList descr;
    if (r.value("trash").toBool())
        descr << tr("deleted").toUpper();

    if (!unitType.isEmpty() && unitType != unitName && unitType != "UAV")
        descr << unitType;
    if (!confName.isEmpty())
        descr << confName;
    if (!notes.isEmpty())
        descr << notes;
    QStringList value;
    if (!unitName.isEmpty())
        value << unitName;
    if (!total.isEmpty())
        value << total;

    j["title"] = stime;
    j["descr"] = descr.join(" - ");
    j["value"] = value.join(' ');

    emit modelInfo(_id, j);

    // provide parsed file info JSON object
    QJsonObject info = QJsonDocument::fromJson(r.value("info").toByteArray()).object();
    emit recordInfo(_id, info, notes);

    return true;
}

bool TelemetryModelTrash::run(QSqlQuery &query)
{
    // toggle trash flag
    query.prepare("SELECT * FROM Telemetry WHERE key=?");
    query.addBindValue(_id);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    auto trash = !query.value("trash").toBool();
    if (trash) {
        qDebug() << "Moving record to trash" << _id << query.value("file").toString();
    } else {
        qDebug() << "Restoring record from trash" << _id << query.value("file").toString();
    }

    query.prepare("UPDATE Telemetry SET trash=? WHERE key=?");
    query.addBindValue(trash ? 1 : QVariant());
    query.addBindValue(_id);
    if (!query.exec())
        return false;

    emit dbModified();

    return true;
}

bool TelemetryLoadFile::run(QSqlQuery &query)
{
    //check for invalid cache (telemetry hash is null)
    query.prepare("SELECT * FROM Telemetry WHERE key=?");
    query.addBindValue(_id);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    const auto basename = query.value("file").toString();
    if (basename.isEmpty()) {
        qWarning() << "empty file name";
        // TODO convert DB data to file
        return false;
    }

    if (query.value("trash").toBool()) {
        qDebug() << "Recovering record from trash";
        auto req = new TelemetryModelTrash(_id, false);
        req->exec(); // chain next
    }

    const auto hash = query.value("hash").toString();

    auto filePath = Session::telemetryFilePath(basename);

    if (!_reader.open(filePath))
        return false;

    emit fileOpened(filePath);

    if (!_reader.parse_payload()) {
        qWarning() << "File corrputed:" << basename;
    }

    bool still_writing = _reader.is_still_writing();
    if (still_writing) {
        qDebug() << "File is still writing";
    }

    // update db record when needed
    do {
        const auto &rinfo = _reader.info();

        QVariantMap q;
        if (!still_writing) {
            // hash only set on finished file
            auto new_hash = rinfo["hash"].toString();
            if (new_hash.isEmpty()) {
                qWarning() << "empty hash";
                break;
            }
            if (hash == new_hash) // no changes
                break;
            q["hash"] = new_hash;
        }

        q["duration"] = rinfo["duration"].toInteger();
        q["size"] = rinfo["size"].toInteger();
        q["info"] = QJsonDocument(rinfo).toJson(QJsonDocument::Compact);
        q["parsed"] = QDateTime::currentMSecsSinceEpoch();
        q["sync"] = 2; // info synced

        // write changes to db
        if (recordUpdateQuery(query, q, "Telemetry", "WHERE key=?")) {
            query.addBindValue(_id);
            if (!query.exec())
                return false;

            qDebug() << "Record hash updated";

            emit dbModified();
        }
    } while (0);

    // read record info
    return TelemetryLoadInfo::run(query);
}

bool TelemetryWriteInfo::run(QSqlQuery &query)
{
    bool bMod = restore;
    if (recordUpdateQuery(query, info, "Telemetry", "WHERE key=?")) {
        query.addBindValue(telemetryID);
        if (!query.exec())
            return false;
        bMod = true;
    }
    if (restore) {
        query.prepare("UPDATE Telemetry SET trash=NULL WHERE key=?");
        query.addBindValue(telemetryID);
        if (!query.exec())
            return false;
    }
    if (bMod) {
        emit dbModified();
    }
    return true;
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
                QFile::moveToTrash(fi.absoluteFilePath());
                continue;
            }
            const auto hash = reader.get_hash();
            const auto time_utc = reader.timestamp();
            const auto utc_offset = reader.utc_offset();
            const auto info = reader.info();

            // check file name
            auto timestamp = QDateTime::fromMSecsSinceEpoch(time_utc);
            timestamp.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(utc_offset));
            auto unitName = info["unit"]["name"].toString();
            if (unitName.isEmpty()) {
                auto info = Session::telemetryFilenameParse(fi.absoluteFilePath());
                unitName = info["unitName"].toString();
            }

            auto basename = Session::telemetryFileBasename(timestamp, unitName);
            auto destFilePath = Session::telemetryFilePath(basename);
            if (fi.absoluteFilePath() != destFilePath) {
                apxConsole() << tr("Rename %1 to %2")
                                    .arg(fi.fileName(), QFileInfo(destFilePath).fileName());
                // check if same file exists
                if (QFile::exists(destFilePath)) {
                    qWarning() << "File exists" << destFilePath;
                    auto hash2 = TelemetryFileReader(destFilePath).get_hash();
                    if (hash == hash2) {
                        qDebug() << "Removing duplicate" << fi.fileName();
                        QFile::moveToTrash(fi.absoluteFilePath());
                        destFilePath.clear();
                        continue;
                    } else {
                        // different files with the same name
                        // rename with some name suffix
                        if (fi.completeBaseName().startsWith(basename)) {
                            // don't rename if already renamed with suffix
                            destFilePath.clear();
                        } else {
                            destFilePath = Session::telemetryFilePathUnique(basename);
                        }
                    }
                }
                if (!destFilePath.isEmpty()) {
                    if (!QFile::rename(fi.absoluteFilePath(), destFilePath)) {
                        apxMsgW() << tr("Failed to rename").append(':') << fi.fileName();
                    } else {
                        fi.setFile(destFilePath);
                    }
                }
            }
            basename = fi.completeBaseName();

            // find file in db and check hash
            bool parse = true;
            quint64 key;
            query.prepare("SELECT * FROM Telemetry WHERE file=?");
            query.addBindValue(basename);
            if (!query.exec())
                break;

            if (!query.next()) {
                qWarning() << "Missing db record" << basename;
                // create record
                if (!TelemetryCreateRecord(time_utc, basename, info, false).run(query))
                    break;

                query.prepare("SELECT key FROM Telemetry WHERE file=?");
                query.addBindValue(basename);
                if (!query.exec() || !query.next())
                    break;
                key = query.value(0).toULongLong();

            } else {
                key = query.value("key").toULongLong();
                // check hash, unit, conf etc
                auto db_hash = query.value("hash").toString();
                if (db_hash != hash) {
                    qWarning() << "Hash mismatch" << basename;
                } else {
                    parse = false;
                }
            }

            // update record if needed
            if (parse) {
                if (!TelemetryLoadFile(key).run(query)) {
                    // file corrupted
                    // continue;
                }
            }

            // just set synced=parsed
            query.prepare("UPDATE Telemetry SET sync=2 WHERE key=?");
            query.addBindValue(key);
            if (!query.exec())
                break;
        }

        // find recods without files (sync is reset)
        query.prepare("SELECT COUNT(*) FROM Telemetry WHERE sync IS NULL");
        if (!query.exec() || !query.next())
            break;
        quint64 cnt = query.value(0).toULongLong();
        if (cnt > 0) {
            apxMsgW() << tr("Missing files").append(':') << cnt;
            // remove records without files
            query.prepare("DELETE FROM Telemetry WHERE sync IS NULL");
            if (!query.exec())
                break;
        }

        // success
        apxMsg() << tr("Telemetry files synced");
        rv = true;
    } while (0);
    emit progress(-1);
    db->enable();
    return rv;
}

bool TelemetryExport::run(QSqlQuery &query)
{
    auto fiSrc = QFileInfo(_src);
    auto fiDest = QFileInfo(_dst);
    if (!fiSrc.exists()) {
        apxMsgW() << tr("Missing data source").append(':') << fiSrc.absoluteFilePath();
        return false;
    }

    emit progress(0);
    bool rv = false;

    do {
        if (_format == telemetry::APXTLM_FTYPE) {
            QFile::remove(fiDest.absoluteFilePath());

            rv = QFile::copy(fiSrc.absoluteFilePath(), fiDest.absoluteFilePath());
            if (!rv) {
                apxMsgW() << tr("Failed to copy").append(':') << fiSrc.absoluteFilePath();
            }
            break;
        }

        if (_format == "csv") {
            QFile::remove(fiDest.absoluteFilePath());

            TelemetryFileReader reader(fiSrc.absoluteFilePath());
            connect(&reader,
                    &TelemetryFileReader::progressChanged,
                    this,
                    &TelemetryExport::progress);

            if (!reader.open()) {
                apxMsgW() << tr("Failed to open").append(':') << fiSrc.absoluteFilePath();
                break;
            }

            // get all fields
            QStringList fields;
            fields << "time";
            fields << "uplink";
            {
                auto c = connect(&reader, &TelemetryFileReader::field, [&fields](QString name) {
                    fields << name;
                });
                reader.parse_payload();
                disconnect(c);
            }
            if (fields.size() <= 2) {
                apxMsgW() << tr("No fields found");
                break;
            }

            // write output stream
            QFile file(fiDest.absoluteFilePath());
            if (!file.open(QFile::WriteOnly | QFile::Text)) {
                apxMsgW() << tr("Failed to write").append(':') << fiDest.absoluteFilePath();
                break;
            }
            QTextStream out(&file);
            out << fields.join(',') << '\n';

            QStringList values;
            for (auto const &field : fields) {
                values << "0";
            }

            connect(&reader,
                    &TelemetryFileReader::values,
                    [&out,
                     &values](quint64 timestamp_ms, TelemetryFileReader::Values data, bool uplink) {
                        for (auto const &value : data.asKeyValueRange()) {
                            auto index = value.first + 2;
                            if (index >= values.size())
                                continue;
                            values[index] = value.second.toString();
                        }
                        values[0] = QString::number(timestamp_ms);
                        values[1] = uplink ? "1" : "0";
                        out << values.join(',') << '\n';
                    });
            reader.parse_payload();
            rv = true;
            break;
        }

        apxMsgW() << tr("Unsupported format").append(':') << _format;
        break;
    } while (0);

    // export success
    if (rv) {
        apxMsg() << tr("Exported").append(':') << fiDest.fileName();
    }

    emit progress(-1);
    return rv;
}
