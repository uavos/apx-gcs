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

    QVariantMap filterFields(QString tableName, QVariantMap values) const;

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

    static QStringList fieldNames(QSqlQuery &query);
    static QVariantList values(QSqlQuery &query, const QStringList &names);
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
