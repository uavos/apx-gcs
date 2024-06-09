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
#include "TelemetryDBReq.h"

#include <App/AppDirs.h>
#include <App/AppRoot.h>

bool DBReqTelemetryCreateRecord::run(QSqlQuery &query)
{
    // qDebug() << vehicleUID << callsign << t;

    query.prepare("INSERT INTO Telemetry(time, file, vehicleUID, callsign, comment, trash) "
                  "VALUES(?, ?, ?, ?, ?, ?)");
    const auto &info = _info;
    query.addBindValue(_t);
    query.addBindValue(_fileName);
    query.addBindValue(info["vehicle"]["uid"].toVariant());
    query.addBindValue(info["vehicle"]["callsign"].toVariant());
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

bool DBReqTelemetryModelRecordsList::run(QSqlQuery &query)
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

bool DBReqTelemetryModelRecordInfo::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM Telemetry WHERE key=?");
    query.addBindValue(_id);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    auto r = query.record();

    QJsonObject j;

    QString time = QDateTime::fromMSecsSinceEpoch(r.value("time").toLongLong())
                       .toString("yyyy MMM dd hh:mm:ss");
    QString callsign = r.value("callsign").toString();
    QString comment = r.value("comment").toString();
    QString notes = r.value("notes").toString();
    QString total;
    quint64 t = r.value("totalTime").toULongLong();
    if (t > 0)
        total = AppRoot::timeToString(t / 1000);
    QStringList descr;
    if (r.value("trash").toBool())
        descr << tr("deleted").toUpper();

    if (!comment.isEmpty())
        descr << comment;
    if (!notes.isEmpty())
        descr << notes;
    QStringList value;
    if (!callsign.isEmpty())
        value << callsign;
    if (!total.isEmpty())
        value << total;

    j["title"] = time;
    j["descr"] = descr.join(" - ");
    j["value"] = value.join(' ');

    emit recordInfo(_id, j);

    return true;
}

bool DBReqTelemetryModelTrash::run(QSqlQuery &query)
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

bool DBReqTelemetryLoadFile::run(QSqlQuery &query)
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
        return false;
    }

    // if (query.value("trash").toBool()) {
    //     qDebug() << "Recovering record from trash";
    //     auto req = new DBReqTelemetryModelTrash(_id, false);
    //     req->exec(); // chain next
    // }

    const auto trash = query.value("trash").toBool();
    auto dir = AppDirs::telemetry();
    if (trash)
        dir.cd("trash");

    auto filePath = dir.absoluteFilePath(file + '.' + telemetry::APXTLM_FTYPE);

    if (!_reader.open(filePath))
        return false;

    if (!_reader.parse_payload())
        return false;

    return true;
}
