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
#include "StorageDB.h"
#include "Database.h"
#include <App/AppDirs.h>

StorageDB::StorageDB(QObject *parent, QString sessionName)
    : DatabaseSession(parent, "storage", sessionName, {}, AppDirs::storage())
{
    new DBReqMakeTable(this,
                       "Telemetry",
                       {
                           "key INTEGER PRIMARY KEY NOT NULL",

                           // fields written on file creation
                           "time INTEGER",    // [ms since epoch] file creation time
                           "vehicleUID TEXT", // vehicle UID from linked table
                           "callsign TEXT",   // vehicle title or conf title
                           "comment TEXT",    // conf name or comment
                           "file TEXT",       // file name with data (no ext)

                           // info extracted from file
                           "size INTEGER",     // [bytes] file size
                           "hash TEXT",        // file content hash
                           "info TEXT",        // JSON info meta data
                           "duration INTEGER", // [ms] total time of telemetry data

                           // local record status and flags
                           "trash INTEGER",    // not null if record deleted
                           "src INTEGER",      // source of record: 0=record, i=import, 2=share
                           "imported INTEGER", // [ms since epoch] file import time
                           "parsed INTEGER",   // [ms since epoch] file parsing time
                           "notes TEXT",       // user notes or comments if any
                       });

    new DBReqMakeIndex(this, "Telemetry", "time", false);
    new DBReqMakeIndex(this, "Telemetry", "vehicleUID", false);
    new DBReqMakeIndex(this, "Telemetry", "callsign", false);
    new DBReqMakeIndex(this, "Telemetry", "file", true);
    new DBReqMakeIndex(this, "Telemetry", "trash", false);
}

DBReqStorage::DBReqStorage()
    : DatabaseRequest(Database::instance()->storage)
{}
