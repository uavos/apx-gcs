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
#include "DatabaseSession.h"
#include "Database.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppRoot.h>

// TODO implement file names format without subfolders to include version number

DatabaseSession::DatabaseSession(
    QObject *parent, const QString &name, const QString &sessionName, QString version, QDir dir)
    : Fact(Database::instance(), name, "", "", Group)
    , sessionName(sessionName)
    , inTransaction(false)
    , sql(QSqlDatabase::addDatabase("QSQLITE", sessionName))
    , database(Database::instance())
    , m_worker(nullptr)
    , m_capacity(0)
{
    Q_UNUSED(parent)
    connect(App::instance(), &App::aboutToQuit, this, &Fact::deleteFact);

    setIcon("database");

    // storage directory
    if (!dir.exists())
        dir.mkpath(".");

    // file name
    fileName = dir.absoluteFilePath(name);
    if (!version.isEmpty())
        fileName.append('.').append(version);
    fileName.append(".db");

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
        query.exec("PRAGMA foreign_keys = ON;");
        query.exec("PRAGMA page_size = 4096;");
        query.exec("PRAGMA cache_size = 16384;");
        query.exec("PRAGMA synchronous = OFF;");
        query.exec("PRAGMA temp_store = DEFAULT;");
        query.exec("PRAGMA locking_mode = NORMAL;");
        query.exec("PRAGMA journal_mode = WAL;");
        query.exec("PRAGMA auto_vacuum = NONE;");
        connect(this, &DatabaseSession::destroyed, [sessionName]() {
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

int DatabaseSession::queueSize()
{
    return m_worker->queueSize();
}

void DatabaseSession::modifiedNotify()
{
    modifiedTimer.start();
}

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
    setValue(size);
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

QStringList DatabaseSession::tableFields(QString tableName) const
{
    return _tableFields.value(tableName);
}
void DatabaseSession::updateTableFields(const QString tableName, QStringList fields)
{
    _tableFields.insert(tableName, fields);
}

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

void DatabaseSession::disable()
{
    QReadLocker locker(&m_workerLock);
    setEnabled(false);
}
void DatabaseSession::enable()
{
    QReadLocker locker(&m_workerLock);
    setEnabled(true);
}
void DatabaseSession::request(DatabaseRequest *req)
{
    QReadLocker locker(&m_workerLock);
    if (!sql.isOpen() || !m_worker || !enabled())
        return;
    m_worker->request(req);
}

void DatabaseSession::vacuumTriggered()
{
    auto req = new db::Vacuum(this);
    connect(
        req,
        &DatabaseRequest::finished,
        this,
        [this]() { f_vacuum->setProgress(-1); },
        Qt::QueuedConnection);
    f_vacuum->setProgress(0);
    req->exec();
}

void DatabaseSession::analyzeTriggered()
{
    auto req = new db::Analyze(this);
    connect(
        req,
        &DatabaseRequest::finished,
        this,
        [this]() { f_analyze->setProgress(-1); },
        Qt::QueuedConnection);
    f_analyze->setProgress(0);
    req->exec();
}

quint64 DatabaseSession::capacity() const
{
    return m_capacity;
}
