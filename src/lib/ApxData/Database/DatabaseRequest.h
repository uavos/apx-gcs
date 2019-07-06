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
#ifndef DatabaseRequest_H
#define DatabaseRequest_H
//=============================================================================
#include <QtCore>
#include <QtSql>
class DatabaseSession;
//=============================================================================
class DatabaseRequest : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseRequest(DatabaseSession *db);
    explicit DatabaseRequest(DatabaseSession *db,
                             const QString &queryString,
                             const QVariantList &bindValues = QVariantList());
    ~DatabaseRequest();

    enum Status {
        Success = 0,
        Error,
        Discarded,
    };
    Q_ENUM(Status)

    virtual void exec();
    bool execSynchronous();
    bool isSynchronous;

    void finish(bool error);

    bool discarded();

    virtual bool run(QSqlQuery &query);

    struct Records
    {
        QStringList names;
        QList<QVariantList> values;
        void clear()
        {
            names.clear();
            values.clear();
        }
    };

    static QVariantMap queryRecord(Records &records, QVariantMap info = QVariantMap(), int i = 0);

    static QVariantMap filterNullValues(QVariantMap values);
    static QVariantMap filterIdValues(QVariantMap values);

protected:
    DatabaseSession *db;
    QString queryString;
    QVariantList bindValues;

    //helpers
    void recordUpdateQuery(QSqlQuery &query,
                           const Records &records,
                           int i,
                           const QString &table,
                           const QString &tail) const;
    void recordInsertQuery(QSqlQuery &query,
                           const Records &records,
                           int i,
                           const QString &table,
                           const QString &tail = QString()) const;

    bool recordUpdateQuery(QSqlQuery &query,
                           QVariantMap values,
                           const QString &table,
                           const QString &tail) const;
    bool recordInsertQuery(QSqlQuery &query,
                           QVariantMap values,
                           const QString &table,
                           const QString &tail = QString()) const;

    QStringList fieldNames(QSqlQuery &query) const;
    QVariantList values(QSqlQuery &query, const QStringList &names) const;
    Records queryRecords(QSqlQuery &query) const;
    QVariantMap queryRecord(QSqlQuery &query, QVariantMap info = QVariantMap()) const;

    void getHash(QCryptographicHash &h, const Records &records) const;
    void getHash(QCryptographicHash &h, const QVariantMap &map) const;
    void getHash(QCryptographicHash &h, QSqlQuery &query) const;

private:
    std::atomic_bool m_discarded;
    std::promise<bool> m_finishedPromise;

public slots:
    void discard();

signals:
    void finished(DatabaseRequest::Status status);

    void queryResults(DatabaseRequest::Records records);
    void dbModified();
};
//=============================================================================
#endif
