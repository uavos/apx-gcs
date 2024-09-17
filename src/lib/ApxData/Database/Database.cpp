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
#include "Database.h"
#include "MissionsDB.h"
#include "TelemetryDB.h"
#include "VehiclesDB.h"

#include <App/App.h>
#include <App/AppLog.h>

APX_LOGGING_CATEGORY(DatabaseLog, "core.Database")

Database *Database::_instance = nullptr;

Database::Database(Fact *parent)
    : Fact(parent, "db", tr("Database"), tr("Data storage"), Group)
{
    _instance = this;
    setIcon("database");

    //new DatabaseSession(this,"test.db","TestSession");
    qRegisterMetaType<DatabaseRequest::Records>("DatabaseRequest::Records");
    qRegisterMetaType<DatabaseRequest::Status>("DatabaseRequest::Status");
    qRegisterMetaType<QList<QSqlRecord>>("QList<QSqlRecord>");

    vehicles = new VehiclesDB(this, QStringLiteral("NodesDbSession"));
    telemetry = new TelemetryDB(this, QStringLiteral("TelemetryDbSession"));
    missions = new MissionsDB(this, QStringLiteral("MissionsDbSession"));
}

Database::~Database()
{
    _instance = nullptr;
}
