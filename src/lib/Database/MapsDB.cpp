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
#include "MapsDB.h"
#include <AppDirs.h>
#include <TileLoader.h>
//=============================================================================
MapsDB::MapsDB(QObject *parent, QString sessionName)
  : DatabaseConnection(parent,AppDirs::db().absoluteFilePath("maps.db"),sessionName)
{
  if(!isOpen())return;
  QSqlQuery query(*this);
  bool ok=transaction(query);
  while(ok){
    //-------------------------------------------------------------
    ok=createTable(query,
          "TileProviders", QStringList()
          <<"key INTEGER PRIMARY KEY NOT NULL"
          <<"name TEXT NOT NULL"
          <<"descr TEXT"
          <<"format TEXT" //data format
    );
    if(!ok)break;

    //-------------------------------------------------------------
    ok=createTable(query,
          "Tiles", QStringList()
          <<"key INTEGER PRIMARY KEY NOT NULL"
          <<"providerID INTEGER NOT NULL"
          <<"hash INTEGER NOT NULL"
          <<"tile BLOB"
          <<"size INTEGER"
          <<"date INTEGER DEFAULT 0"  //date downloaded
          <<"FOREIGN KEY(providerID) REFERENCES TileProviders(key) ON DELETE CASCADE ON UPDATE CASCADE"
    );
    if(!ok)break;
    ok=createIndex(query,"Tiles","hash",false);
    if(!ok)break;
    ok=createIndex(query,"Tiles","providerID,hash",true);
    if(!ok)break;


    //-------------------------------------------------------------
    //sync providers
    query.prepare("SELECT * FROM TileProviders");
    ok=query.exec();
    if(!ok)break;
    providersMap.clear();
    QMetaEnum metaEnum = QMetaEnum::fromType<TileLoader::MapID>();
    QStringList pnames;
    for(int i=0;i<metaEnum.keyCount();++i){
      pnames<<metaEnum.key(i);
    }
    while(query.next()){
      QString s=query.value(1).toString();
      if(!pnames.contains(s)){
        qWarning()<<"deprecated tile provider maps DB"<<s;
        continue;
      }
      pnames.removeOne(s);
      providersMap.insert((TileLoader::MapID)metaEnum.keyToValue(s.toUtf8().data()),query.value(0).toULongLong());
    }
    //qDebug()<<pnames;
    foreach (QString s, pnames) {
      query.prepare("INSERT INTO TileProviders(name) VALUES(?)");
      query.addBindValue(s);
      ok=query.exec();
      if(!ok)break;
      providersMap.insert((TileLoader::MapID)metaEnum.keyToValue(s.toUtf8().data()),query.lastInsertId().toULongLong());
      qDebug()<<"add provider maps DB"<<s;
    }

    break;
  }
  checkResult(query);
}
//=============================================================================
bool MapsDB::writeTile(quint8 mapID, quint64 hash, const QByteArray &tile)
{
  if(!isOpen())return false;
  QSqlQuery query(*this);
  bool ok=true;
  uint t=QDateTime::currentDateTime().toTime_t();
  while(ok){
    //insert tile record
    query.prepare(
      "INSERT INTO Tiles"
      "(providerID, hash, tile, size, date) "
      "VALUES(?, ?, ?, ?, ?)");
    query.addBindValue(providersMap.value(mapID));
    query.addBindValue(hash);
    query.addBindValue(tile);
    query.addBindValue(tile.size());
    query.addBindValue(t);
    ok=query.exec();
    break;
  }
  return ok;
}
//=============================================================================
QByteArray MapsDB::readTile(quint8 mapID, quint64 hash)
{
  if(!isOpen())return QByteArray();
  QSqlQuery query(*this);
  query.prepare("SELECT tile FROM Tiles WHERE providerID = ? AND hash = ?");
  query.addBindValue(providersMap.value(mapID));
  query.addBindValue(hash);
  if(!(query.exec() && query.next())){
    return QByteArray();
  }
  return query.value(0).toByteArray();
}
//=============================================================================
