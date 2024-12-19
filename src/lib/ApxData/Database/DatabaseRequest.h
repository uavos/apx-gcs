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

#include <QtCore>
#include <QtSql>
class DatabaseSession;

class DatabaseRequest : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseRequest(DatabaseSession *db,
                             const QString &queryString = {},
                             const QVariantList &bindValues = {});
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

protected:
    DatabaseSession *db;
    QString queryString;
    QVariantList bindValues;

    //helpers
    bool recordUpdateQuery(QSqlQuery &query,
                           const QJsonObject &jso,
                           const QString &table,
                           const QString &tail) const;

    static QJsonObject record_to_json(const QSqlRecord &record, const QStringList &names);

    void getHash(QCryptographicHash &h, const QJsonValue &jsv) const;
    void getHash(QCryptographicHash &h, QSqlQuery &query) const;

private:
    std::atomic_bool m_discarded;
    std::promise<bool> m_finishedPromise;

public slots:
    void discard();

signals:
    void discardRequested();
    void finished(DatabaseRequest::Status status);
    void success();

    void queryResults(QJsonArray records);
    void dbModified();
};
