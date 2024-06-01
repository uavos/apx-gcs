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
#include "TelemetryReqWrite.h"
#include "Database.h"

#include <Mandala/Mandala.h>

bool DBReqTelemetryNewRecord::run(QSqlQuery &query)
{
    // qDebug() << vehicleUID << callsign << t;

    query.prepare(
        "INSERT INTO Telemetry(vehicleUID, callsign, comment, trash, time) VALUES(?, ?, ?, ?, ?)");
    query.addBindValue(vehicleUID);
    query.addBindValue(callsign);
    query.addBindValue(comment);
    query.addBindValue(recording ? QVariant() : 1);
    query.addBindValue(t);
    if (!query.exec())
        return false;
    telemetryID = query.lastInsertId().toULongLong();
    emit idUpdated(telemetryID);
    emit dbModified();
    //qDebug() << telemetryID;
    return true;
}

bool DBReqTelemetryWriteData::run(QSqlQuery &query)
{
    // qDebug() << telemetryID << t << _values;

    if (!telemetryID) {
        qWarning() << "missing telemetryID";
        return false;
    }

    auto d = static_cast<TelemetryDB *>(db);

    for (auto [uid, value] : _values) {
        auto fkey = d->field_key(uid);
        if (!fkey) {
            qWarning() << "missing mandala uid" << uid;
            continue;
        }

        if (uplink) {
            query.prepare("INSERT INTO TelemetryUplink"
                          "(telemetryID, fieldID, time, value) "
                          "VALUES(?, ?, ?, ?)");
        } else {
            query.prepare("INSERT INTO TelemetryDownlink"
                          "(telemetryID, fieldID, time, value) "
                          "VALUES(?, ?, ?, ?)");
        }
        query.addBindValue(telemetryID);
        query.addBindValue(fkey);
        query.addBindValue(t);
        query.addBindValue(value);
        if (!query.exec()) {
            qWarning() << telemetryID << fkey;
            return false;
        }
    }
    return true;
}

bool DBReqTelemetryWriteEvent::run(QSqlQuery &query)
{
    // qDebug() << telemetryID << t << name << value;
    query.prepare("INSERT INTO TelemetryEvents"
                  "(telemetryID, time, name, value, uid, uplink) "
                  "VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(telemetryID);
    query.addBindValue(t);
    query.addBindValue(name);
    query.addBindValue(value);
    query.addBindValue(uid);
    query.addBindValue(uplink ? 1 : QVariant());
    if (!query.exec())
        return false;
    return true;
}

bool DBReqTelemetryWriteInfo::run(QSqlQuery &query)
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

bool DBReqTelemetryWriteSharedInfo::run(QSqlQuery &query)
{
    info = filterNullValues(info);
    query.prepare("SELECT * FROM TelemetryShare"
                  " WHERE telemetryID=?");
    query.addBindValue(telemetryID);
    if (!query.exec())
        return false;
    if (query.next()) {
        quint64 sharedID = query.value(0).toULongLong();
        if (recordUpdateQuery(query, info, "TelemetryShare", "WHERE key=?")) {
            query.addBindValue(sharedID);
            if (!query.exec())
                return false;
        }
        return true;
    }
    info["telemetryID"] = telemetryID;
    if (recordInsertQuery(query, info, "TelemetryShare")) {
        if (!query.exec())
            return false;
    }
    return true;
}
