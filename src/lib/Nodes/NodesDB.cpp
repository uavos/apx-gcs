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
#include "NodesDB.h"
#include "Nodes.h"
#include "Vehicle.h"
//=============================================================================
NodesDB::NodesDB(Nodes *parent)
  : QObject(parent), nodes(parent), m_enabled(true)
{
  createTables();
}
//=============================================================================
bool NodesDB::checkResult(QSqlQuery &query)
{
  if(query.lastError().type()==QSqlError::NoError){
    FactSystem::db()->commit();
    return true;
  }
  qWarning() << "NodesDB SQL error:" << query.lastError().text();
  qWarning() << query.executedQuery();
  return false;
}
//=============================================================================
void NodesDB::createTables()
{
  QSqlQuery query(*FactSystem::db());
  while(FactSystem::db()->isOpen()){
    //table of actual state of all nodes ever seen
    query.prepare(
      "CREATE TABLE IF NOT EXISTS Nodes ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "sn TEXT NOT NULL UNIQUE, "
      "name TEXT, "
      "version TEXT, "
      "hardware TEXT, "
      "date INTEGER DEFAULT 0"  //date seen
      ")");
    if(!query.exec())break;
    query.prepare("CREATE UNIQUE INDEX IF NOT EXISTS idx_Nodes_sn ON Nodes (sn);");
    if(!query.exec())break;

    //nodes cache (current structure)
    query.prepare(
      "CREATE TABLE IF NOT EXISTS NodesCache ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "nodeID INTEGER NOT NULL, "
      "hash TEXT, "
      "params INTEGER, "
      "commands INTEGER, "
      "FOREIGN KEY(nodeID) REFERENCES Nodes(key) ON DELETE CASCADE"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesCache_nodeID ON NodesCache (nodeID);");
    if(!query.exec())break;
    query.prepare(
      "CREATE TABLE IF NOT EXISTS NodesCacheDict ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "cacheID INTEGER NOT NULL, "
      "id INTEGER, "
      "name TEXT, "
      "title TEXT, "
      "descr TEXT, "
      "units TEXT, "
      "defValue TEXT, "
      "ftype TEXT, "
      "array INTEGER, "
      "opts TEXT, "
      "sect TEXT, "
      "FOREIGN KEY(cacheID) REFERENCES NodesCache(key) ON DELETE CASCADE"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesCacheDict_cacheID ON NodesCacheDict (cacheID);");
    if(!query.exec())break;

    //table of parameters files (journal)
    query.prepare(
      "CREATE TABLE IF NOT EXISTS NodesData ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "nodeID INTEGER NOT NULL, "
      "date INTEGER DEFAULT 0, "
      "hash TEXT, "     //values hash
      "name TEXT, "     //node name
      "version TEXT, "  //node version
      "comment TEXT, "  //comment or file name
      "FOREIGN KEY(nodeID) REFERENCES Nodes(key) ON DELETE CASCADE"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesData_nodeID ON NodesData (nodeID);");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesData_date ON NodesData (date);");
    if(!query.exec())break;
    //table of parameters values (journal)
    query.prepare(
      "CREATE TABLE IF NOT EXISTS NodesDataValues ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "dataID INTEGER NOT NULL, "
      "name TEXT NOT NULL, "
      "value TEXT, "
      "FOREIGN KEY(dataID) REFERENCES NodesData(key) ON DELETE CASCADE"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesDataValues_dataID ON NodesDataValues (dataID);");
    if(!query.exec())break;


    break;
  }
  checkResult(query);
}
//=============================================================================
quint64 NodesDB::nodeGetID(NodeItem *node, QSqlQuery *query, bool *ok)
{
  query->prepare("SELECT key FROM Nodes WHERE sn = ?");
  query->addBindValue(node->sn.toHex().toUpper());
  if(!(query->exec() && query->next() && (!query->value(0).isNull()))){
    *ok=false;
    return 0;
  }
  *ok=true;
  return query->value(0).toULongLong();
}
//=============================================================================
//=============================================================================
void NodesDB::nodeInfoWrite(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->infoValid())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  //register node sn
  query.prepare("INSERT INTO Nodes(sn) VALUES(?)");
  query.addBindValue(node->sn.toHex().toUpper());
  query.exec();
  bool ok=true;
  while(ok){
    quint64 nodeID=nodeGetID(node,&query,&ok);
    if(!ok)break;
    //modify node info
    query.prepare("UPDATE Nodes SET name=?, version=?, hardware=? WHERE key=?");
    query.addBindValue(node->title());
    query.addBindValue(node->version());
    query.addBindValue(node->hardware());
    query.addBindValue(nodeID);
    ok=query.exec();
    if(!ok)break;
    if(node->lastSeenTime){
      query.prepare("UPDATE Nodes SET date=? WHERE key=?");
      query.addBindValue(node->lastSeenTime);
      query.addBindValue(nodeID);
      ok=query.exec();
      if(!ok)break;
    }
    break;
  }
  checkResult(query);
}
void NodesDB::nodeInfoRead(NodeItem *node)
{
  if(!m_enabled)return;
  if(node->infoValid())return;
  if(!FactSystem::db()->isOpen())return;
  QSqlQuery query(*FactSystem::db());
  query.setForwardOnly(true);
  //read node name
  query.prepare("SELECT name, hardware FROM Nodes WHERE sn = ?");
  query.addBindValue(node->sn.toHex().toUpper());
  if(!(query.exec() && query.next()))return;
  if(!query.value(0).isNull())node->setTitle(query.value(0).toString());
  if(!query.value(1).isNull())node->setHardware(query.value(1).toString());
  //qDebug()<<title()<<hardware();
}
//=============================================================================
//=============================================================================
void NodesDB::nodeDictWrite(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->dictValid())return;
  if(node->conf_hash.isEmpty())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  bool ok=true;
  quint64 nodeID=nodeGetID(node,&query,&ok);
  if(!ok)return;
  //check for existing cache record
  query.prepare("SELECT hash, params, commands FROM NodesCache WHERE nodeID = ?");
  query.addBindValue(nodeID);
  if(!(query.exec() && query.next()))return;
  if(query.value(0).isNull() || query.value(1).isNull() || query.value(2).isNull())return;
  while(1){
    if(query.value(0).toString()!=node->conf_hash)break;
    if(query.value(1).toInt()!=node->allFields.size())break;
    if(query.value(2).toInt()!=node->commands.cmd.size())break;
    //cache already present
    return;
  }
  //create new cache
  while(ok){
    //remove old node cache with same hash
    query.prepare("DELETE FROM NodesCache WHERE nodeID=?");
    query.addBindValue(nodeID);
    query.exec();
    //create cache record
    query.prepare(
      "INSERT INTO NodesCache("
      "nodeID, hash, params, commands"
      ") VALUES(?, ?, ?, ?)");
    query.addBindValue(nodeID);
    query.addBindValue(node->conf_hash);
    query.addBindValue(node->allFields.size());
    query.addBindValue(node->commands.cmd.size());
    ok=query.exec();
    if(!ok)break;
    quint64 cacheID=query.lastInsertId().toULongLong();
    //save new dict cache
    foreach (NodeField *f, node->allFields) {
      query.prepare(
        "INSERT INTO NodesCacheDict("
        "cacheID, id, name, title, descr, units, defValue, ftype, array, opts, sect"
        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
      query.addBindValue(cacheID);
      query.addBindValue(f->id);
      query.addBindValue(f->name());
      query.addBindValue(f->title());
      query.addBindValue(f->descr());
      query.addBindValue(f->units());
      query.addBindValue(f->defaultValue());
      query.addBindValue(f->ftypeString());
      query.addBindValue(f->array());
      query.addBindValue(f->enumStrings().join(','));
      query.addBindValue(f->fpath());
      ok=query.exec();
      if(!ok)break;
    }
    if(!ok)break;
    for (int i=0;i<node->commands.cmd.size();i++){
      query.prepare(
        "INSERT INTO NodesCacheDict("
        "cacheID, id, name, title, descr, ftype"
        ") VALUES(?, ?, ?, ?, ?, ?)");
      query.addBindValue(cacheID);
      query.addBindValue(node->commands.cmd.at(i));
      query.addBindValue(node->commands.name.at(i));
      query.addBindValue(node->commands.name.at(i));
      query.addBindValue(node->commands.descr.at(i));
      query.addBindValue("command");
      ok=query.exec();
      if(!ok)break;
    }
    break;
  }
  qDebug()<<"cache updated"<<node->title();
  checkResult(query);
}
void NodesDB::nodeDictRead(NodeItem *node)
{
  if(!m_enabled)return;
  if(node->dictValid())return;
  if(!FactSystem::db()->isOpen())return;
  QSqlQuery query(*FactSystem::db());
  query.setForwardOnly(true);
  bool ok=true;
  quint64 nodeID=nodeGetID(node,&query,&ok);
  if(!ok)return;
  //read cache ID
  query.prepare("SELECT key, hash, params, commands FROM NodesCache WHERE nodeID = ?");
  query.addBindValue(nodeID);
  if(!(query.exec() && query.next()))return;
  if(query.value(0).isNull() || query.value(1).isNull() || query.value(2).isNull() || query.value(3).isNull())return;
  if(query.value(1).toString()!=node->conf_hash)return;
  quint64 cacheID=query.value(0).toULongLong();
  int pCnt=query.value(2).toInt();
  int cCnt=query.value(3).toInt();
  //read conf structure
  query.prepare("SELECT * FROM NodesCacheDict WHERE cacheID = ?");
  query.addBindValue(cacheID);
  if(!query.exec())return;
  if(!query.isActive())return;
  int paramsCnt=0,commandsCnt=0;
  while(query.next()) {
    //qDebug()<<query.value("name");
    QVariant v=query.value("id");
    if(v.isNull())return;
    QString s=query.value("ftype").toString();
    if(s=="command"){
      node->commands.cmd.append(v.toUInt());
      node->commands.name.append(query.value("name").toString());
      node->commands.descr.append(query.value("descr").toString());
      commandsCnt++;
      continue;
    }
    NodeField *f=node->allFields.value(v.toInt(),NULL);
    if(!f)return;
    if(f->ftype>=0)return;
    for(int i=0;i<255;i++){
      QString sft=f->ftypeString(i);
      if(sft.isEmpty())break;
      if(sft!=s)continue;
      f->ftype=i;
      break;
    }
    if(f->ftype<0)return;
    f->setName(query.value("name").toString());
    f->setTitle(query.value("title").toString());
    f->setDescr(query.value("descr").toString());
    f->setUnits(query.value("units").toString());
    f->setDefaultValue(query.value("defValue").toString());
    f->setArray(query.value("array").toUInt());
    f->setEnumStrings(query.value("opts").toString().split(',',QString::SkipEmptyParts));
    f->groups=query.value("sect").toString().split('/',QString::SkipEmptyParts);
    //qDebug()<<f->path();
    paramsCnt++;
  }
  if(paramsCnt!=pCnt || commandsCnt!=cCnt){
    node->clearCommands();
    if(paramsCnt>0 || commandsCnt>0)qDebug()<<"cache error";
    return;
  }
  node->commands.valid=true;
  foreach (NodeField *f, node->allFields) {
    f->setDictValid(true);
  }
  if(!node->dictValid()){
    node->clearCommands();
    node->setDictValid(false); //recursive
  }else{
    qDebug()<<"cache read"<<node->title();
  }
}
void NodesDB::nodeDictClear(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->dictValid())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  bool ok=true;
  quint64 nodeID=nodeGetID(node,&query,&ok);
  if(!ok)return;
  query.prepare("DELETE FROM NodesCache WHERE nodeID=?");
  query.addBindValue(nodeID);
  query.exec();
  qDebug()<<"cache clear"<<node->path();
  checkResult(query);
}
//=============================================================================
//=============================================================================
void NodesDB::nodeDataWrite(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->dataValid())return;
  if(node->reconf())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  uint t=QDateTime::currentDateTime().toTime_t();
  bool ok=true;
  while(ok){
    //find nodeID
    quint64 nodeID=nodeGetID(node,&query,&ok);
    if(!ok)break;
    //check data hash (most recent)
    query.prepare("SELECT key, date, hash FROM NodesData WHERE nodeID = ? ORDER BY date DESC LIMIT 1");
    query.addBindValue(nodeID);
    ok=query.exec();
    if(!ok)break;
    QString hash=node->hash().toHex().toUpper();
    if(query.next() && query.value(2).toString()==hash){
      qDebug()<<"same data hash";
      //modify timestamp
      quint64 dataID=query.value(0).toULongLong();
      query.prepare("UPDATE NodesData SET date=?, name=?, version=? WHERE key = ?");
      query.addBindValue(t);
      query.addBindValue(node->title());
      query.addBindValue(node->version());
      query.addBindValue(dataID);
      ok=query.exec();
      break; //finish
    }
    //create new data record
    query.prepare(
      "INSERT INTO NodesData("
      "nodeID, date, hash, name, version, comment"
      ") VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(nodeID);
    query.addBindValue(t);
    query.addBindValue(hash);
    query.addBindValue(node->title());
    query.addBindValue(node->version());
    query.addBindValue(node->status());
    ok=query.exec();
    if(!ok)break;
    quint64 dataID=query.lastInsertId().toULongLong();
    //create values table
    query.prepare("DROP TABLE t_NodesDataNew");
    query.exec();
    query.prepare(
      //"DROP TABLE t_NodesDataNew;"
      "CREATE TEMPORARY TABLE t_NodesDataNew ("
      "key INTEGER PRIMARY KEY, "
      "name TEXT, "
      "value TEXT"
      ")");
    if(!query.exec())return;
    QHash<QString,NodeField*> fmap=node->allFieldsDataMap();
    foreach (const QString s, fmap.keys()) {
      //insert new record
      query.prepare("INSERT INTO t_NodesDataNew(name, value) VALUES(?, ?)");
      query.addBindValue(s);
      query.addBindValue(fmap.value(s)->text());
      ok=query.exec();
      if(!ok)break;
    }
    if(!ok)break;
    //insert values update table
    query.prepare(
      "INSERT INTO NodesDataValues"
      "(dataID, name, value) SELECT ? AS dataID, name, value FROM "
      "(SELECT name, value FROM t_NodesDataNew EXCEPT SELECT name, value FROM ("
      "SELECT name, value, MAX(date) FROM"
      " (SELECT v.key, v.name, v.value, d.date "
      " FROM (NodesDataValues AS v INNER JOIN (SELECT * FROM NodesData WHERE nodeID=?) d ON v.dataID=d.key ) "
      " ) GROUP BY name "
      ")"
      ")"
      );
    query.addBindValue(dataID);
    query.addBindValue(nodeID);
    ok=query.exec();
    if(!ok)break;
    qDebug()<<"new data";
    break;
  }
  //qDebug()<<"data saved"<<path();
  checkResult(query);
}
void NodesDB::nodeDataRead(NodeItem *node, quint64 dataID)
{
  if(!m_enabled)return;
  if(!node->dataValid())return;
  if(!FactSystem::db()->isOpen())return;
  QSqlQuery query(*FactSystem::db());
  bool ok=true;
  while(ok){
    //read date to filter
    query.prepare("SELECT nodeID, date FROM NodesData WHERE key = ?");
    query.addBindValue(dataID);
    ok=query.exec() && query.next();
    if(!ok)break;
    quint64 nodeID=query.value(0).toULongLong();
    uint date=query.value(1).toUInt();
    //build values table
    query.prepare(
      "SELECT name, value, MAX(date) FROM"
      " (SELECT v.key, v.name, v.value, d.date "
      " FROM (NodesDataValues AS v INNER JOIN (SELECT * FROM NodesData WHERE nodeID=? AND date<=?) d ON v.dataID=d.key ) "
      " ) GROUP BY name "
      );
    query.addBindValue(nodeID);
    query.addBindValue(date);
    ok=query.exec();
    if(!ok)break;
    QHash<QString,NodeField*> fmap=node->allFieldsDataMap();
    while(query.next()){
      const QString fname=query.value(0).toString();
      //find field by name
      foreach (const QString s, fmap.keys()) {
        if(s!=fname)continue;
        fmap.value(s)->setValue(query.value(1).toString());
        break;
      }
    }
    break;
  }
  qDebug()<<"data restored"<<node->title();
  checkResult(query);
}
NodesDB::NodeDataKeys NodesDB::nodeDataReadKeys(NodeItem *node, int limit)
{
  NodeDataKeys keys;
  if(!m_enabled)return keys;
  if(!node->dataValid())return keys;
  if(!FactSystem::db()->isOpen())return keys;
  QSqlQuery query(*FactSystem::db());
  bool ok=true;
  while(ok){
    //find nodeID
    quint64 nodeID=nodeGetID(node,&query,&ok);
    if(!ok)break;
    //check data hash (most recent)
    query.prepare("SELECT key, date, version, comment FROM NodesData WHERE nodeID = ? ORDER BY date DESC LIMIT ?");
    query.addBindValue(nodeID);
    query.addBindValue(limit);
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      QPair<QString,quint64> k;
      uint t=query.value(1).toUInt();
      QString s=QDateTime::fromTime_t(t).toString("yyyy MMM dd hh:mm:ss");
      QString sc=query.value(3).toString();
      QString sver=query.value(2).toString();
      if((!sver.isEmpty()) && sver!=node->version()){
        sc+=QString(sc.isEmpty()?"%1":" v%1").arg(query.value(2).toString());
      }
      if(!sc.isEmpty())s+=QString(" (%1)").arg(sc);
      k.first=s;
      k.second=query.value(0).toULongLong();
      keys.append(k);
    }
    break;
  }
  //qDebug()<<"data saved"<<path();
  checkResult(query);
  return keys;
}
//=============================================================================
//=============================================================================
