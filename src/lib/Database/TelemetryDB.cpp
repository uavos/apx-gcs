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
#include "TelemetryDB.h"
#include <AppDirs.h>
#include <Vehicles.h>
#include <Vehicle.h>
#include <VehicleMandala.h>
#include <VehicleMandalaFact.h>
//=============================================================================
TelemetryDB::TelemetryDB(QObject *parent, QString sessionName, Vehicle *vehicle, bool readOnly)
  : DatabaseConnection(parent,AppDirs::db().absoluteFilePath("telemetry.db"),sessionName,readOnly)
{
  if(!isOpen())return;
  QSqlQuery query(*this);
  bool ok=transaction(query);
  while(ok){
    ok=createTable(query,
          "Telemetry", QStringList()
          <<"key INTEGER PRIMARY KEY NOT NULL"
          <<"vehicleUID TEXT"
          <<"callsign TEXT"
          <<"comment TEXT"
          <<"notes TEXT"
          <<"rec INTEGER"
          <<"timestamp INTEGER"  //[ms since epoch]
    );
    if(!ok)break;
    ok=createIndex(query,"Telemetry","vehicleUID");
    if(!ok)break;
    ok=createTable(query,
          "TelemetryFields", QStringList()
          <<"key INTEGER PRIMARY KEY NOT NULL"
          <<"id INTEGER"
          <<"name TEXT"
          <<"title TEXT"
          <<"descr TEXT"
          <<"units TEXT"
          <<"opts TEXT"
          <<"sect TEXT"
    );
    if(!ok)break;
    ok=createTable(query,
          "TelemetryDownlink", QStringList()
          <<"telemetryID INTEGER NOT NULL"
          <<"fieldID INTEGER NOT NULL"
          <<"time INTEGER"  //[ms since file]
          <<"value REAL"
          <<"FOREIGN KEY(fieldID) REFERENCES TelemetryFields(key) ON DELETE CASCADE ON UPDATE CASCADE"
          <<"FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON DELETE CASCADE ON UPDATE CASCADE"
    );
    if(!ok)break;
    ok=createIndex(query,"TelemetryDownlink","telemetryID");
    if(!ok)break;
    ok=createTable(query,
          "TelemetryUplink", QStringList()
          <<"telemetryID INTEGER NOT NULL"
          <<"fieldID INTEGER NOT NULL"
          <<"time INTEGER"  //[ms since file]
          <<"value REAL"
          <<"FOREIGN KEY(fieldID) REFERENCES TelemetryFields(key) ON DELETE CASCADE ON UPDATE CASCADE"
          <<"FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON DELETE CASCADE ON UPDATE CASCADE"
    );
    if(!ok)break;
    ok=createIndex(query,"TelemetryUplink","telemetryID");
    if(!ok)break;
    ok=createTable(query,
          "TelemetryEvents", QStringList()
          <<"telemetryID INTEGER NOT NULL"
          <<"time INTEGER"  //[ms since file]
          <<"uplink INTEGER"
          <<"name TEXT"
          <<"value TEXT"
          <<"data BLOB"
          <<"FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON DELETE CASCADE ON UPDATE CASCADE"
    );
    if(!ok)break;
    ok=createIndex(query,"TelemetryEvents","telemetryID");

    //sync data fields
    query.prepare("SELECT * FROM TelemetryFields");
    ok=query.exec();
    if(!ok)break;
    recFacts.clear();
    VehicleMandala *m=vehicle?vehicle->f_mandala:Vehicles::instance()->f_local->f_mandala;
    QStringList fnames=m->names;
    while(query.next()){
      QString s=query.value(2).toString();
      if(!fnames.contains(s)){
        qWarning()<<"deprecated field telemetry DB"<<s;
        continue;
      }
      fnames.removeOne(s);
      recFacts.insert(static_cast<Fact*>(m->child(s)),query.value(0).toULongLong());
    }
    foreach (QString s, fnames) {
      Fact *f=static_cast<Fact*>(m->child(s));
      query.prepare("INSERT INTO TelemetryFields(name) VALUES(?)");
      query.addBindValue(f->name());
      ok=query.exec();
      if(!ok)break;
      recFacts.insert(f,query.lastInsertId().toULongLong());
      qDebug()<<"add field telemetry DB"<<s;
    }
    foreach (Fact *f, recFacts.keys()) {
      quint64 fieldID=recFacts.value(f);
      query.prepare(
        "UPDATE TelemetryFields SET "
        "name=?, title=?, descr=?, units=?, opts=?, sect=? "
        "WHERE key=?");
      query.addBindValue(f->name());
      query.addBindValue(f->title());
      query.addBindValue(f->descr());
      query.addBindValue(f->units());
      query.addBindValue(f->enumStrings().join(','));
      query.addBindValue(QVariant());
      query.addBindValue(fieldID);
      ok=query.exec();
      if(!ok)break;
    }

    break;
  }
  checkResult(query);
}
//=============================================================================
quint64 TelemetryDB::writeRecord(const QString &vehicleUID, const QString &callsign, const QString &comment, bool rec, quint64 timestamp)
{
  if(!isOpen())return 0;
  QSqlQuery query(*this);
  query.prepare("INSERT INTO Telemetry(vehicleUID, callsign, comment, rec, timestamp) VALUES(?, ?, ?, ?, ?)");
  query.addBindValue(vehicleUID);
  query.addBindValue(callsign);
  query.addBindValue(comment);
  query.addBindValue(rec?1:QVariant());
  query.addBindValue(timestamp);
  if(!query.exec())return 0;
  return query.lastInsertId().toULongLong();
}
//=============================================================================
bool TelemetryDB::writeEvent(quint64 telemetryID, quint64 timestamp, const QString &name, const QString &value, bool uplink, const QByteArray &data)
{
  if(!isOpen())return false;
  QSqlQuery query(*this);
  bool ok=true;
  while(ok){
    //insert event record
    query.prepare(
      "INSERT INTO TelemetryEvents"
      "(telemetryID, time, uplink, name, value, data) "
      "VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(telemetryID);
    query.addBindValue(timestamp);
    query.addBindValue(uplink?1:QVariant());
    query.addBindValue(name);
    query.addBindValue(value);
    query.addBindValue(data.isEmpty()?QVariant():data);
    ok=query.exec();
    break;
  }
  return ok;
}
//=============================================================================
bool TelemetryDB::writeDownlink(quint64 telemetryID, quint64 timestamp, const QList<Fact *> &facts)
{
  //18:10=0kb 18:25=5Mb 20Mb/hr 18:30=6Mb 18Mb/hr
  //bus: 19:05=17Mb 19:10=19.2Mb 26.4Mb/hr
  if(!isOpen())return false;
  QSqlQuery query(*this);
  bool ok=facts.size()>1?transaction(query):true;
  //insert data records for facts
  for(int i=0;ok && i<facts.size();++i){
    Fact *f=facts.at(i);
    ok=writeField(query,telemetryID,timestamp,recFacts.value(f),f->value(),false);
    if(!ok)break;
  }
  return checkResult(query);
}
//=============================================================================
bool TelemetryDB::writeField(quint64 telemetryID, quint64 timestamp, quint64 fieldID, const QVariant &v, bool uplink)
{
  if(!isOpen())return false;
  QSqlQuery query(*this);
  writeField(query,telemetryID,timestamp,fieldID,v,uplink);
  return checkResult(query);
}
//=============================================================================
bool TelemetryDB::writeField(QSqlQuery &query, quint64 telemetryID, quint64 timestamp, quint64 fieldID, const QVariant &v, bool uplink)
{
  //insert data record
  if(uplink){
    query.prepare(
      "INSERT INTO TelemetryUplink"
      "(telemetryID, fieldID, time, value) "
      "VALUES(?, ?, ?, ?)");
  }else{
    query.prepare(
      "INSERT INTO TelemetryDownlink"
      "(telemetryID, fieldID, time, value) "
      "VALUES(?, ?, ?, ?)");
  }
  query.addBindValue(telemetryID);
  query.addBindValue(fieldID);
  query.addBindValue(timestamp);
  query.addBindValue(v);
  return query.exec();
}
//=============================================================================
bool TelemetryDB::writeNotes(quint64 telemetryID, const QString &s)
{
  if(!isOpen())return false;
  QSqlQuery query(*this);
  query.prepare("UPDATE Telemetry SET notes=? WHERE key = ?");
  query.addBindValue(s);
  query.addBindValue(telemetryID);
  query.exec();
  return checkResult(query);
}
//=============================================================================
bool TelemetryDB::deleteRecord(quint64 telemetryID)
{
  if(!isOpen())return false;
  QSqlQuery query(*this);
  query.prepare("UPDATE Telemetry SET rec=NULL WHERE key = ?");
  query.addBindValue(telemetryID);
  query.exec();
  return checkResult(query);
}
//=============================================================================
//=============================================================================
//=============================================================================
bool TelemetryDB::createTelemetryTable(quint64 telemetryID)
{
  if(!isOpen())return false;
  QSqlQuery query(*this);
  bool ok=transaction(query);
  while(ok){
    //create temp table
    query.prepare("DROP TABLE t_TelemetryData");
    query.exec();
    query.prepare(
      "CREATE TEMPORARY TABLE t_TelemetryData AS "
      "SELECT * FROM ( "
      "SELECT time, fieldID, value, NULL AS uplink, NULL AS name, NULL AS data FROM TelemetryDownlink WHERE telemetryID=? "
      "UNION ALL "
      "SELECT time, fieldID, value, 1 AS uplink, NULL AS name, NULL AS data FROM TelemetryUplink WHERE telemetryID=? "
      "UNION ALL "
      "SELECT time, NULL AS fieldID, value, uplink, name, data FROM TelemetryEvents WHERE telemetryID=? "
      ") ORDER BY time"
      );
    query.addBindValue(telemetryID);
    query.addBindValue(telemetryID);
    query.addBindValue(telemetryID);
    ok=query.exec();
    if(!ok)break;

    break;
  }
  return checkResult(query);
}
//=============================================================================
bool TelemetryDB::readDownlink(quint64 telemetryID, quint64 time)
{
  if(!isOpen())return false;
  QSqlQuery query(*this);
  bool ok=true;
  while(ok){
    //collect all facts values up to time
    query.prepare(
      "SELECT fieldID, value, max(time) FROM TelemetryDownlink "
      "WHERE telemetryID=? AND time<=? GROUP BY fieldID"
      );
    query.addBindValue(telemetryID);
    query.addBindValue(time);
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      quint64 fieldID=query.value(0).toULongLong();
      VehicleMandalaFact *f=qobject_cast<VehicleMandalaFact*>(recFacts.key(fieldID));
      if(!f)continue;
      f->setValueLocal(query.value(1));
    }

    break;
  }
  return checkResult(query);
}
//=============================================================================
//=============================================================================
