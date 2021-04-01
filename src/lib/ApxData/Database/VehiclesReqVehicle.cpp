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
#include "VehiclesReqVehicle.h"
#include "VehiclesReqDict.h"
#include "VehiclesReqNconf.h"

bool DBReqSaveVehicleInfo::run(QSqlQuery &query)
{
    QString uid = info.value("uid").toString();
    QVariant vuid;
    if (uid == QString(uid.size(), QChar('0')))
        uid.clear();
    if (!uid.isEmpty()) {
        vuid = uid;
        if (info.value("class").isNull())
            info["class"] = "UAV";
    }
    if (!info.value("time").toULongLong())
        info["time"] = t;

    //find the latest existing record by UID
    if (vuid.isNull()) {
        query.prepare("SELECT * FROM Vehicles WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Vehicles WHERE uid=? ORDER BY time DESC LIMIT 1");
        query.addBindValue(vuid);
    }
    if (!query.exec())
        return false;
    while (query.next()) {
        quint64 latestTime = query.value("time").toULongLong();
        //merge ident from previous registrations
        if (info.value("callsign").toString().isEmpty())
            info["callsign"] = query.value("callsign");
        if (vuid.isNull()) {
            //only one record with null uid (LOCAL)
            vehicleID = query.value(0).toULongLong();
        } else {
            //find exact matching unique record
            query.prepare("SELECT * FROM Vehicles WHERE uid=? AND callsign=? AND class=?");
            query.addBindValue(vuid);
            query.addBindValue(info.value("callsign"));
            query.addBindValue(info.value("class"));
            if (!query.exec())
                return false;
            if (!query.next())
                break; //create new
            vehicleID = query.value(0).toULongLong();
            qDebug() << "vehicle exists" << info.value("callsign").toString();
        }
        //same vehicle found - update data if newer
        if (latestTime >= info.value("time").toULongLong())
            return true;
        query.prepare("UPDATE Vehicles SET time=? WHERE key=?");
        query.addBindValue(info.value("time"));
        query.addBindValue(vehicleID);
        if (!query.exec())
            return false;
        emit foundID(vehicleID);
        return true;
    }

    //register new vehicle record
    qDebug() << "new vehicle" << info.value("callsign").toString();

    query.prepare("INSERT INTO Vehicles(uid,callsign,class,time) VALUES(?,?,?,?)");
    query.addBindValue(vuid);
    query.addBindValue(info.value("callsign"));
    query.addBindValue(info.value("class"));
    query.addBindValue(info.value("time"));
    if (!query.exec())
        return false;
    vehicleID = query.lastInsertId().toULongLong();
    emit foundID(vehicleID);
    return true;
}
