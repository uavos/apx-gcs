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
#include "DatabaseConnection.h"
QList<DatabaseConnection*> DatabaseConnection::connections;
//=============================================================================
DatabaseConnection::DatabaseConnection(QObject *parent, QString fileName, QString sessionName, bool readOnly)
  : QObject(parent),
    QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", sessionName)),
    _sessionName(sessionName),
    _transaction(false)
{
  setDatabaseName(fileName);
  QStringList opts;
  opts.append("QSQLITE_ENABLE_SHARED_CACHE");
  opts.append("QSQLITE_BUSY_TIMEOUT=10000000");
  if(readOnly) opts.append("QSQLITE_OPEN_READONLY");
  setConnectOptions(opts.join("; "));
  if(!open()){
    qWarning()<<lastError();
    QSqlDatabase::removeDatabase(sessionName);
  }else{
    QSqlQuery query(*this);
    query.exec("PRAGMA page_size = 4096;");
    query.exec("PRAGMA cache_size = 16384;");
    query.exec("PRAGMA synchronous = OFF;");
    query.exec("PRAGMA temp_store = DEFAULT;");
    query.exec("PRAGMA locking_mode = NORMAL;");
    query.exec("PRAGMA journal_mode = WAL;");
    connections.append(this);
    connect(this,&DatabaseConnection::destroyed,[=](){
      QSqlDatabase::removeDatabase(sessionName);
      //qDebug()<<sessionName<<"removed";
    });
  }
}
DatabaseConnection::~DatabaseConnection()
{
  connections.removeAll(this);
  close();
  //qDebug()<<_sessionName<<"closed";
}
//=============================================================================
bool DatabaseConnection::createTable(QSqlQuery &query, const QString &tableName, const QStringList &fields)
{
  const QString &s=QString("CREATE TABLE IF NOT EXISTS %1 (%2)").arg(tableName).arg(fields.join(','));
  query.prepare(s);
  return query.exec();
}
//=============================================================================
bool DatabaseConnection::createIndex(QSqlQuery &query, const QString &tableName, const QString &indexName, bool unique)
{
  const QString &s=QString("CREATE%3 INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%4);").arg(tableName).arg(QString(indexName).replace(',','_')).arg(unique?" UNIQUE":"").arg(indexName);
  query.prepare(s);
  return query.exec();
}
//=============================================================================
bool DatabaseConnection::transaction(QSqlQuery &query)
{
  query.prepare("BEGIN IMMEDIATE TRANSACTION");
  _transaction=query.exec();
  //qDebug()<<"transaction"<<_transaction;
  return _transaction;
}
//=============================================================================
bool DatabaseConnection::commit(QSqlQuery &query)
{
  //qDebug()<<"commit"<<_transaction;
  if(!_transaction)return true;
  _transaction=false;
  if(query.lastError().type()!=QSqlError::NoError){
    query.prepare("ROLLBACK");
  }else{
    query.prepare("COMMIT");
  }
  return query.exec();
}
//=============================================================================
bool DatabaseConnection::checkResult(QSqlQuery &query, bool silent)
{
  if(query.lastError().type()!=QSqlError::NoError){
    if(!silent){
      qWarning() << "SQL error:" << query.lastError().text() << query.executedQuery();
    }
    commit(query);
    return false;
  }
  commit(query);
  return true;
}
//=============================================================================
