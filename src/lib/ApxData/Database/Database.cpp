/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "Database.h"
#include "MissionsDB.h"
#include "NodesDB.h"
#include "TelemetryDB.h"

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

    nodes = new NodesDB(this, QStringLiteral("NodesDbSession"));
    telemetry = new TelemetryDB(this, QStringLiteral("TelemetryDbSession"));
    missions = new MissionsDB(this, QStringLiteral("MissionsDbSession"));
}
