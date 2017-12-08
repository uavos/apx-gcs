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
      "date INTEGER DEFAULT 0, "
      "name TEXT, "
      "version TEXT, "
      "hardware TEXT, "
      "hash TEXT, "
      "params INTEGER, "
      "commands INTEGER"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE UNIQUE INDEX IF NOT EXISTS idx_Nodes_sn ON Nodes (sn);");
    if(!query.exec())break;
    //table of node fields (structure)
    query.prepare(
      "CREATE TABLE IF NOT EXISTS NodesDict ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "sn TEXT NOT NULL, "
      "hash TEXT NOT NULL, "
      "date INTEGER DEFAULT 0, "
      "id INTEGER, "
      "name TEXT, "
      "title TEXT, "
      "descr TEXT, "
      "units TEXT, "
      "defValue TEXT, "
      "ftype TEXT, "
      "array INTEGER, "
      "opts TEXT, "
      "sect TEXT"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesDict_sn ON NodesDict (sn);");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesDict_hash ON NodesDict (hash);");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesDict ON NodesDict (sn,hash);");
    if(!query.exec())break;
    //table of parameters values (journal)
    query.prepare(
      "CREATE TABLE IF NOT EXISTS NodesData ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "sn TEXT NOT NULL, "
      "hash TEXT, "
      "vhash TEXT NOT NULL, "
      "date INTEGER DEFAULT 0, "
      "version TEXT, "
      "name TEXT NOT NULL, "
      "type TEXT NOT NULL, "
      "value TEXT"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesData_sn ON NodesData (sn);");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesData_hash ON NodesData (hash);");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesData_vhash ON NodesData (vhash);");
    if(!query.exec())break;
    break;
  }
  checkResult(query);
}
//=============================================================================
//=============================================================================
void NodesDB::nodeInfoRead(NodeItem *node)
{
  if(!m_enabled)return;
  if(node->infoValid())return;
  if(!FactSystem::db()->isOpen())return;
  QSqlQuery query(*FactSystem::db());
  //read node name
  query.prepare("SELECT name, hardware FROM Nodes WHERE sn = ?");
  query.addBindValue(node->sn.toHex().toUpper());
  if(!(query.exec() && query.next()))return;
  if(!query.value(0).isNull())node->setTitle(query.value(0).toString());
  if(!query.value(1).isNull())node->setHardware(query.value(1).toString());
  //qDebug()<<title()<<hardware();
}
void NodesDB::nodeInfoWrite(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->infoValid())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  uint t=QDateTime::currentDateTime().toTime_t();
  //register node sn
  query.prepare("INSERT INTO Nodes(sn, date) VALUES(?, ?)");
  query.addBindValue(node->sn.toHex().toUpper());
  query.addBindValue(t);
  query.exec();
  //modify node info
  query.prepare("UPDATE Nodes SET date=?, name=?, version=?, hardware=? WHERE sn=?");
  query.addBindValue(t);
  query.addBindValue(node->title());
  query.addBindValue(node->version());
  query.addBindValue(node->hardware());
  query.addBindValue(node->sn.toHex().toUpper());
  query.exec();

  checkResult(query);
}
//=============================================================================
void NodesDB::nodeDictRead(NodeItem *node)
{
  if(!m_enabled)return;
  if(node->dictValid())return;
  if(!FactSystem::db()->isOpen())return;
  QSqlQuery query(*FactSystem::db());
  //qDebug()<<"cache lookup"<<title();
  //read params cnt
  query.prepare("SELECT params, commands, hash FROM Nodes WHERE sn = ?");
  query.addBindValue(node->sn.toHex().toUpper());
  if(!(query.exec() && query.next()))return;
  if(query.value(0).isNull() || query.value(1).isNull() || query.value(2).isNull())return;
  int pCnt=query.value(0).toInt();
  int cCnt=query.value(1).toInt();
  if(query.value(2).toString()!=node->conf_hash)return;;
  //read conf structure
  query.prepare("SELECT * FROM NodesDict WHERE sn = ? AND hash = ?");
  query.addBindValue(node->sn.toHex().toUpper());
  query.addBindValue(node->conf_hash);
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
    node->commands.cmd.clear();
    node->commands.name.clear();
    node->commands.descr.clear();
    if(paramsCnt>0 || commandsCnt>0)qDebug()<<"cache error";
    return;
  }
  node->commands.valid=true;
  foreach (NodeField *f, node->allFields) {
    f->setDictValid(true);
  }
  //qDebug()<<"cache read"<<title();
}
void NodesDB::nodeDictWrite(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->dictValid())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  //modify node conf hash
  query.prepare("UPDATE Nodes SET date=?, hash=?, params=?, commands=? WHERE sn=?");
  query.addBindValue(QDateTime::currentDateTime().toTime_t());
  query.addBindValue(node->conf_hash);
  query.addBindValue(node->allFields.size());
  query.addBindValue(node->commands.cmd.size());
  query.addBindValue(node->sn.toHex().toUpper());
  query.exec();
  //save all fields structure
  query.prepare("DELETE FROM NodesDict WHERE sn=? AND hash=?");
  query.addBindValue(node->sn.toHex().toUpper());
  query.addBindValue(node->conf_hash);
  query.exec();
  uint t=QDateTime::currentDateTime().toTime_t();
  foreach (NodeField *f, node->allFields) {
    query.prepare(
      "INSERT INTO NodesDict("
      "sn, hash, date, id, name, title, descr, units, defValue, ftype, array, opts, sect"
      ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(node->sn.toHex().toUpper());
    query.addBindValue(node->conf_hash);
    query.addBindValue(t);
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
    if(!query.exec())break;
  }
  for (int i=0;i<node->commands.cmd.size();i++){
    query.prepare(
      "INSERT INTO NodesDict("
      "sn, hash, date, id, name, title, descr, ftype"
      ") VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(node->sn.toHex().toUpper());
    query.addBindValue(node->conf_hash);
    query.addBindValue(t);
    query.addBindValue(node->commands.cmd.at(i));
    query.addBindValue(node->commands.name.at(i));
    query.addBindValue(node->commands.name.at(i));
    query.addBindValue(node->commands.descr.at(i));
    query.addBindValue("command");
    if(!query.exec())break;
  }
  //qDebug()<<"cache updated"<<path();
  checkResult(query);
}
void NodesDB::nodeDictClear(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->dictValid())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  query.prepare("DELETE FROM NodesDict WHERE sn=? AND hash=?");
  query.addBindValue(node->sn.toHex().toUpper());
  query.addBindValue(node->conf_hash);
  query.exec();
  qDebug()<<"cache clear"<<node->path();
  checkResult(query);
}
//=============================================================================
void NodesDB::nodeDataRead(NodeField *f)
{
  if(!m_enabled)return;
}
//=============================================================================
void NodesDB::nodeDataWrite(NodeItem *node)
{
  if(!m_enabled)return;
  if(!node->dataValid())return;
  if(!FactSystem::db()->isOpen())return;
  FactSystem::db()->transaction();
  QSqlQuery query(*FactSystem::db());
  uint t=QDateTime::currentDateTime().toTime_t();
  const QString vhash=node->hash().toHex().toUpper();
  const QString sn=node->sn.toHex().toUpper();
  //check existing
  query.prepare("DELETE FROM NodesData WHERE sn=? AND hash=? AND vhash=?");
  query.addBindValue(sn);
  query.addBindValue(node->conf_hash);
  query.addBindValue(vhash);
  query.exec();
  QList<NodeField*> list;
  for(int i=0;i<node->allFields.size();++i){
    NodeField *f=node->allFields.at(i);
    //expand complex numbers
    list.clear();
    bool bComplex=f->size()>0;
    QString fnamePrefix;
    if(bComplex){
      fnamePrefix=f->name()+"/";
      for(int i2=0;i2<f->size();++i2){
        list<< static_cast<NodeField*>(f->childItems().at(i2));
      }
    }else list<<f;
    for(int i2=0;i2<list.size();++i2){
      NodeField *fx=list.at(i2);
      //insert new record
      query.prepare(
        "INSERT INTO NodesData("
        "sn, hash, vhash, date, version, name, type, value"
        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
      query.addBindValue(sn);
      query.addBindValue(node->conf_hash);
      query.addBindValue(vhash);
      query.addBindValue(t);
      query.addBindValue(node->version());
      query.addBindValue(fnamePrefix+fx->name());
      query.addBindValue(QMetaEnum::fromType<Fact::DataType>().valueToKey(fx->dataType()));
      query.addBindValue(fx->text());
      query.exec();
    }
  }
  //qDebug()<<"data saved"<<path();
  checkResult(query);
}
//=============================================================================
