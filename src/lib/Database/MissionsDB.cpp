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
#include "MissionsDB.h"
#include <AppDirs.h>
#include <Vehicles.h>
#include <Vehicle.h>
#include <VehicleMandala.h>
#include <VehicleMandalaFact.h>
//=============================================================================
MissionsDB::MissionsDB(QObject *parent, QString sessionName, bool readOnly)
  : DatabaseConnection(parent,AppDirs::db().absoluteFilePath("missions.db"),sessionName,readOnly)
{
  if(!isOpen())return;
  QSqlQuery query(*this);
  bool ok=transaction(query);
  while(ok){
    ok=createTable(query,
          "Missions", QStringList()
          <<"key INTEGER PRIMARY KEY NOT NULL"
          <<"hash TEXT"
          <<"title TEXT"
          <<"vehicleUID TEXT"
          <<"notes TEXT"
          <<"topLeftLat REAL DEFAULT 0.0"
          <<"topLeftLon REAL DEFAULT 0.0"
          <<"bottomRightLat REAL DEFAULT 0.0"
          <<"bottomRightLon REAL DEFAULT 0.0"
          <<"date INTEGER DEFAULT 0"  //access date
    );
    if(!ok)break;
    ok=createIndex(query,"Missions","vehicleUID");
    if(!ok)break;

    ok=createTable(query,
          "Runways", QStringList()
          <<"key INTEGER PRIMARY KEY NOT NULL"
          <<"lat REAL"
          <<"lon REAL"
          <<"hmsl INTEGER"
          <<"dN INTEGER"
          <<"dE INTEGER"
          <<"title TEXT"          //default
          <<"appLength INTEGER"   //default
          <<"turn STRING"         //default
    );
    if(!ok)break;
    ok=createTable(query,
          "MissionRunways", QStringList()
          <<"missionID INTEGER NOT NULL"
          <<"runwayID INTEGER NOT NULL"
          <<"title TEXT"
          <<"appLength INTEGER"
          <<"turn STRING"
          <<"FOREIGN KEY(missionID) REFERENCES Missions(key) ON DELETE CASCADE ON UPDATE CASCADE"
          <<"FOREIGN KEY(runwayID) REFERENCES Runways(key) ON DELETE CASCADE ON UPDATE CASCADE"
    );
    if(!ok)break;


    ok=createTable(query,
          "Waypoints", QStringList()
          <<"key INTEGER PRIMARY KEY NOT NULL"
          <<"missionID INTEGER NOT NULL"
          <<"order INTEGER"
          <<"lat REAL"
          <<"lon REAL"
          <<"altitude INTEGER"
          <<"type STRING"
          <<"actions STRING"
          <<"FOREIGN KEY(missionID) REFERENCES Missions(key) ON DELETE CASCADE ON UPDATE CASCADE"
    );
    if(!ok)break;
    ok=createIndex(query,"Waypoints","missionID");
    if(!ok)break;

    break;
  }
  checkResult(query);
}
//=============================================================================
//=============================================================================
