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
#pragma once

#include "DatabaseWorker.h"
#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
#include <QMutex>
#include <QtCore>
#include <QtSql>
class Database;

class DatabaseSession : public Fact //, public QSqlDatabase
{
    Q_OBJECT
    Q_PROPERTY(quint64 capacity READ capacity NOTIFY capacityChanged)

public:
    explicit DatabaseSession(QObject *parent,
                             const QString &name,
                             const QString &sessionName,
                             QString version);
    ~DatabaseSession();

    Fact *f_vacuum;
    Fact *f_analyze;

    QString fileName;
    QString sessionName;
    bool inTransaction;

    void disable();
    void enable();

    void request(DatabaseRequest *req);

    bool transaction(QSqlQuery &query);
    bool commit(QSqlQuery &query, bool forceError = false);
    bool rollback(QSqlQuery &query);

    int queueSize();

    QSqlDatabase sql;

    QStringList tableFields(QString tableName) const;
    void updateTableFields(const QString tableName, QStringList fields);

public:
    quint64 capacity() const;

protected:
    quint64 m_capacity;

protected:
    Database *database;
    DatabaseWorker *m_worker;
    QReadWriteLock m_workerLock;

private:
    QTimer modifiedTimer;

    DelayedEvent evtUpdateInfo;
    int infoQueueSize;

    QHash<QString, QStringList> _tableFields;

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

signals:
    void capacityChanged();
};

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

protected:
    bool run(QSqlQuery &query);

private:
    QString tableName;
    QStringList fields;
    QString tail;
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

protected:
    bool run(QSqlQuery &query);

private:
    QString tableName;
    QString indexName;
    bool unique;
};
class DBReqVacuum : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqVacuum(DatabaseSession *db)
        : DatabaseRequest(db)
        , name(QFileInfo(db->fileName).baseName())
    {}

protected:
    bool run(QSqlQuery &query);

private:
    QString name;
};
class DBReqAnalyze : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqAnalyze(DatabaseSession *db)
        : DatabaseRequest(db)
        , name(QFileInfo(db->fileName).baseName())
    {}

protected:
    bool run(QSqlQuery &query);

private:
    QString name;
};
