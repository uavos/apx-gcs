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
    query.prepare("INSERT INTO Telemetry(time, file, unitUID, unitName, unitType, confName, trash) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?)");
    const auto &info = _info;
    query.addBindValue(_t);
    query.addBindValue(_fileName);
    query.addBindValue(info["unit"]["uid"].toVariant());
    query.addBindValue(info["unit"]["name"].toVariant());
    query.addBindValue(info["unit"]["type"].toVariant());
    query.addBindValue(info["conf"].toVariant());
    query.addBindValue(_trash ? 1 : QVariant());
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

    const auto file = query.value("file").toString();
    if (file.isEmpty()) {
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

    auto filePath = Session::telemetryFilePath(file);

    if (!_reader.open(filePath))
        return false;

    emit fileOpened(filePath);

    if (!_reader.parse_payload())
        return false;

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
        q["info"] = QJsonDocument(rinfo).toJson(QJsonDocument::Compact);

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
