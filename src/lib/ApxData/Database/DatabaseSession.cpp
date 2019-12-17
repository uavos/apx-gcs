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
#include "DatabaseSession.h"
#include "Database.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <App/AppRoot.h>
//=============================================================================
DatabaseSession::DatabaseSession(QObject *parent,
                                 const QString &fileName,
                                 const QString &sessionName)
    : Fact(Database::instance(), QFileInfo(fileName).baseName(), "", "", Group)
    , fileName(fileName)
    , sessionName(sessionName)
    , inTransaction(false)
    , sql(QSqlDatabase::addDatabase("QSQLITE", sessionName))
    , database(Database::instance())
    , m_worker(nullptr)
    , m_capacity(0)
{
    setParent(parent);
    connect(parent, &QObject::destroyed, this, &Fact::remove);

    setIcon("database");

    QDir dir(QFileInfo(fileName).absoluteDir());
    if (!dir.exists())
        dir.mkpath(".");

    //QMutexLocker lock(&mutex);
    sql.setDatabaseName(fileName);
    QStringList opts;
    opts.append("QSQLITE_ENABLE_SHARED_CACHE");
    opts.append("QSQLITE_BUSY_TIMEOUT=10000000");
    //if(readOnly) opts.append("QSQLITE_OPEN_READONLY");
    sql.setConnectOptions(opts.join("; "));
    if (!sql.open()) {
        apxMsgW() << sql.lastError();
        QSqlDatabase::removeDatabase(sessionName);
    } else {
        QSqlQuery query(sql);
        query.exec("PRAGMA foreign_keys = on;");
        query.exec("PRAGMA page_size = 4096;");
        query.exec("PRAGMA cache_size = 16384;");
        query.exec("PRAGMA synchronous = OFF;");
        query.exec("PRAGMA temp_store = DEFAULT;");
        query.exec("PRAGMA locking_mode = NORMAL;");
        query.exec("PRAGMA journal_mode = WAL;");
        query.exec("PRAGMA auto_vacuum = NONE;");
        database->add(this);
        connect(this, &DatabaseSession::destroyed, [=]() {
            QSqlDatabase::removeDatabase(sessionName);
            //qDebug()<<"DB"<<sessionName<<"removed";
        });
    }
    m_worker = new DatabaseWorker(this, nullptr);

    modifiedTimer.setSingleShot(true);
    modifiedTimer.setInterval(500);
    connect(&modifiedTimer, &QTimer::timeout, this, &DatabaseSession::modified);

    //info update
    infoQueueSize = 0;
    evtUpdateInfo.forceTimeout = true;
    connect(&evtUpdateInfo, &DelayedEvent::triggered, this, &DatabaseSession::updateInfo);
    connect(m_worker,
            &DatabaseWorker::infoChanged,
            &evtUpdateInfo,
            &DelayedEvent::schedule,
            Qt::QueuedConnection);

    //tools
    f_vacuum = new Fact(this, "vacuum", tr("Optimize"), tr("Compress size"));
    f_vacuum->setIcon("arrow-collapse-vertical");
    connect(f_vacuum, &Fact::triggered, this, &DatabaseSession::vacuumTriggered);
    f_analyze = new Fact(this, "analyze", tr("Analyze"), tr("Check integrity"));
    f_analyze->setIcon("check-all");
    connect(f_analyze, &Fact::triggered, this, &DatabaseSession::analyzeTriggered);

    //capacity
    connect(this, &DatabaseSession::capacityChanged, this, &DatabaseSession::updateDescr);
    connect(this, &Fact::triggered, this, &DatabaseSession::updateCapacity);
    updateCapacity();

    updateInfo();
    updateDescr();
}
DatabaseSession::~DatabaseSession()
{
    m_workerLock.lockForWrite();
    delete m_worker;
    m_worker = nullptr;
    m_workerLock.unlock();
    sql.close();
    //QSqlDatabase::removeDatabase(sessionName);
    //qDebug()<<"DB"<<sessionName<<"closed";
}
//=============================================================================
int DatabaseSession::queueSize()
{
    return m_worker->queueSize();
}
//=============================================================================
void DatabaseSession::modifiedNotify()
{
    modifiedTimer.start();
}
//=============================================================================
void DatabaseSession::updateInfo()
{
    int qsz = queueSize();
    int rate = m_worker->rate();
    QString size = qsz > 0 ? QString("%1 q").arg(qsz) : "";
    if (rate > 0) {
        if (!size.isEmpty())
            size.append(" | ");
        size.append(QString("%1 qps").arg(rate));
    }
    setStatus(size);
    if (rate > 0 || qsz != infoQueueSize)
        evtUpdateInfo.schedule();
    infoQueueSize = qsz;
}
void DatabaseSession::updateDescr()
{
    setDescr(AppRoot::capacityToString(capacity()));
}
void DatabaseSession::updateCapacity()
{
    quint64 v = static_cast<quint64>(QFileInfo(fileName).size());
    if (m_capacity == v)
        return;
    m_capacity = v;
    emit capacityChanged();
}
//=============================================================================
//=============================================================================
bool DatabaseSession::transaction(QSqlQuery &query)
{
    if (!sql.isOpen())
        return false;
    if (inTransaction)
        commit(query);
    query.prepare("BEGIN IMMEDIATE TRANSACTION");
    inTransaction = query.exec();
    //qDebug()<<"transaction"<<inTransaction;
    //if(!inTransaction)mutex.unlock();
    return inTransaction;
}
//=============================================================================
bool DatabaseSession::commit(QSqlQuery &query, bool forceError)
{
    if (!sql.isOpen())
        return false;
    //qDebug()<<"commit"<<inTransaction;
    if (!inTransaction)
        return true;
    inTransaction = false;
    if (forceError || query.lastError().type() != QSqlError::NoError) {
        apxConsoleW() << "SQL error:" << query.lastError().text().append(":")
                      << query.executedQuery();
        query.prepare("ROLLBACK");
    } else {
        query.prepare("COMMIT");
    }
    bool rv = query.exec();
    return rv;
}
bool DatabaseSession::rollback(QSqlQuery &query)
{
    if (!sql.isOpen())
        return false;
    //qDebug()<<"rollback"<<inTransaction;
    if (!inTransaction)
        return true;
    inTransaction = false;
    query.prepare("ROLLBACK");
    bool rv = query.exec();
    return rv;
}
//=============================================================================
void DatabaseSession::request(DatabaseRequest *req)
{
    QReadLocker locker(&m_workerLock);
    if (!sql.isOpen() || !m_worker)
        return;
    m_worker->request(req);
}
//=============================================================================
void DatabaseSession::vacuumTriggered()
{
    DBReqVacuum *req = new DBReqVacuum(this);
    connect(
        req,
        &DatabaseRequest::finished,
        this,
        [this]() { f_vacuum->setProgress(-1); },
        Qt::QueuedConnection);
    f_vacuum->setProgress(0);
    req->exec();
}
//=============================================================================
void DatabaseSession::analyzeTriggered()
{
    DBReqAnalyze *req = new DBReqAnalyze(this);
    connect(
        req,
        &DatabaseRequest::finished,
        this,
        [this]() { f_analyze->setProgress(-1); },
        Qt::QueuedConnection);
    f_analyze->setProgress(0);
    req->exec();
}
//=============================================================================
//=============================================================================
quint64 DatabaseSession::capacity() const
{
    return m_capacity;
}
//=============================================================================
//=============================================================================
//=============================================================================
bool DBReqMakeTable::run(QSqlQuery &query)
{
    query.prepare(QString("PRAGMA table_info('%1')").arg(tableName));
    if (!query.exec())
        return false;
    if (!query.next()) {
        //not exists - create new table
        const QString &s = QString("CREATE TABLE IF NOT EXISTS %1 (%2) %3")
                               .arg(tableName)
                               .arg(fields.join(','))
                               .arg(tail);
        db->transaction(query);
        query.prepare(s);
        if (!query.exec())
            return false;
        db->commit(query);
        return true;
    }
    //update existing table
    do {
        QString s = query.value("name").toString();
        for (int i = 0; i < fields.size(); ++i) {
            if (!fields.at(i).simplified().startsWith(s + " "))
                continue;
            fields.removeAt(i);
            break;
        }
    } while (query.next());
    for (int i = 0; i < fields.size(); ++i) {
        if (!fields.at(i).simplified().startsWith("FOREIGN KEY"))
            continue;
        fields.removeAt(i);
        i--;
    }
    if (!fields.isEmpty()) {
        db->transaction(query);
        for (int i = 0; i < fields.size(); ++i) {
            QString s = QString("ALTER TABLE '%1' ADD %2").arg(tableName).arg(fields.at(i));
            qDebug() << s;
            query.prepare(s);
            if (!query.exec())
                return false;
        }
        apxMsg() << tr("Table %1 updated").arg(tableName);
        db->commit(query);
    }
    return true;
}
//=============================================================================
bool DBReqMakeIndex::run(QSqlQuery &query)
{
    const QString &s = QString("CREATE%3 INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%4);")
                           .arg(tableName)
                           .arg(QString(indexName).replace(',', '_'))
                           .arg(unique ? " UNIQUE" : "")
                           .arg(indexName);
    db->transaction(query);
    query.prepare(s);
    if (!query.exec())
        return false;
    db->commit(query);
    return true;
}
//=============================================================================
bool DBReqVacuum::run(QSqlQuery &query)
{
    QElapsedTimer t0;
    t0.start();
    apxMsg() << tr("Optimizing") << name + "...";
    if (!db->commit(query))
        return false;
    query.prepare("VACUUM");
    if (!query.exec())
        return false;
    apxMsg() << tr("Optimized") << name << t0.elapsed() << "ms";
    return true;
}
//=============================================================================
bool DBReqAnalyze::run(QSqlQuery &query)
{
    QElapsedTimer t0;
    t0.start();
    apxMsg() << tr("Analyzing") << name + "...";
    if (!db->commit(query))
        return false;
    query.prepare("ANALYZE");
    if (!query.exec()) {
        apxMsgW() << tr("Error") << name << t0.elapsed() << "ms";
        apxMsgW() << query.lastError().text();
        return false;
    }
    apxMsg() << "OK" << name << t0.elapsed() << "ms";
    return true;
}
//=============================================================================
