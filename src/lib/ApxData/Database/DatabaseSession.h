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
#ifndef DatabaseSession_H
#define DatabaseSession_H
//=============================================================================
#include "DatabaseWorker.h"
#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
#include <QMutex>
#include <QtCore>
#include <QtSql>
class Database;
//=============================================================================
class DatabaseSession : public Fact //, public QSqlDatabase
{
    Q_OBJECT
    Q_PROPERTY(quint64 capacity READ capacity NOTIFY capacityChanged)

public:
    explicit DatabaseSession(QObject *parent, const QString &fileName, const QString &sessionName);
    ~DatabaseSession();

    Fact *f_vacuum;
    Fact *f_analyze;

    QString fileName;
    QString sessionName;
    bool inTransaction;

    void request(DatabaseRequest *req);

    bool transaction(QSqlQuery &query);
    bool commit(QSqlQuery &query, bool forceError = false);
    bool rollback(QSqlQuery &query);

    int queueSize();

    QSqlDatabase sql;

private:
    QTimer modifiedTimer;

    DelayedEvent evtUpdateInfo;
    int infoQueueSize;

protected:
    Database *database;
    DatabaseWorker *m_worker;
    QReadWriteLock m_workerLock;

private slots:
    void updateInfo();
    void updateDescr();
    void updateCapacity();

    //tools
    void vacuumTriggered();
    void analyzeTriggered();

public slots:
    void modifiedNotify();

signals:
    void modified();

    //-----------------------------------------
    //PROPERTIES
public:
    quint64 capacity() const;

protected:
    quint64 m_capacity;
signals:
    void capacityChanged();
};
//=============================================================================
class DBReqMakeTable : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqMakeTable(DatabaseSession *db,
                            const QString &tableName,
                            const QStringList &fields,
                            const QString &tail = QString())
        : DatabaseRequest(db)
        , tableName(tableName)
        , fields(fields)
        , tail(tail)
    {
        exec();
    }

private:
    QString tableName;
    QStringList fields;
    QString tail;

protected:
    bool run(QSqlQuery &query);
};
class DBReqMakeIndex : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqMakeIndex(DatabaseSession *db,
                            const QString &tableName,
                            const QString &indexName,
                            bool unique)
        : DatabaseRequest(db)
        , tableName(tableName)
        , indexName(indexName)
        , unique(unique)
    {
        exec();
    }

private:
    QString tableName;
    QString indexName;
    bool unique;

protected:
    bool run(QSqlQuery &query);
};
class DBReqVacuum : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqVacuum(DatabaseSession *db)
        : DatabaseRequest(db)
        , name(QFileInfo(db->fileName).baseName())
    {}

private:
    QString name;

protected:
    bool run(QSqlQuery &query);
};
class DBReqAnalyze : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqAnalyze(DatabaseSession *db)
        : DatabaseRequest(db)
        , name(QFileInfo(db->fileName).baseName())
    {}

private:
    QString name;

protected:
    bool run(QSqlQuery &query);
};
//=============================================================================
#endif
