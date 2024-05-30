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
#include "DatabaseRequest.h"
#include "DatabaseSession.h"

DatabaseRequest::DatabaseRequest(DatabaseSession *db)
    : QObject()
    , isSynchronous(false)
    , db(db)
    , m_discarded(false)
{
    connect(this,
            &DatabaseRequest::dbModified,
            db,
            &DatabaseSession::modifiedNotify,
            Qt::QueuedConnection);
}
DatabaseRequest::DatabaseRequest(DatabaseSession *db,
                                 const QString &queryString,
                                 const QVariantList &bindValues)
    : QObject()
    , isSynchronous(false)
    , db(db)
    , queryString(queryString)
    , bindValues(bindValues)
    , m_discarded(false)
{
    if (!db)
        return;

    connect(this,
            &DatabaseRequest::dbModified,
            db,
            &DatabaseSession::modifiedNotify,
            Qt::QueuedConnection);
}

void DatabaseRequest::exec()
{
    if (!db)
        return;

    db->request(this);
}
bool DatabaseRequest::execSynchronous()
{
    if (!db)
        return false;

    isSynchronous = true;
    db->request(this);
    auto future = m_finishedPromise.get_future();
    future.wait();
    return future.get();
}

void DatabaseRequest::finish(bool error)
{
    if (isSynchronous)
        m_finishedPromise.set_value(!error);
    if (discarded())
        emit finished(Discarded);
    else if (error)
        emit finished(Error);
    else
        emit finished(Success);
}

bool DatabaseRequest::run(QSqlQuery &query)
{
    if (queryString.isEmpty())
        return true;
    query.prepare(queryString);
    for (int i = 0; i < bindValues.size(); ++i) {
        query.addBindValue(bindValues.at(i));
    }
    if (!query.exec())
        return false;
    if (discarded())
        return true;
    emit queryResults(queryRecords(query));
    return true;
}

void DatabaseRequest::discard()
{
    if (m_discarded)
        return;
    m_discarded = true;
}

bool DatabaseRequest::discarded()
{
    return m_discarded;
}

void DatabaseRequest::recordUpdateQuery(QSqlQuery &query,
                                        const Records &records,
                                        int i,
                                        const QString &table,
                                        const QString &tail) const
{
    const QStringList &n = records.names;
    const QVariantList &r = records.values.at(i);
    QString s = QString("UPDATE %1 SET %2=? %3").arg(table).arg(n.join("=?,")).arg(tail);
    //qDebug()<<s;
    query.prepare(s);
    for (int i = 0; i < r.size(); ++i) {
        query.addBindValue(r.at(i));
    }
}
void DatabaseRequest::recordInsertQuery(QSqlQuery &query,
                                        const Records &records,
                                        int i,
                                        const QString &table,
                                        const QString &tail) const
{
    const QStringList &n = records.names;
    const QVariantList &r = records.values.at(i);
    QStringList vlist;
    for (int i = 0; i < n.size(); ++i)
        vlist.append("?");
    QString s = QString("INSERT INTO %1(%2) VALUES(%3) %4")
                    .arg(table)
                    .arg(n.join(","))
                    .arg(vlist.join(','))
                    .arg(tail);
    //qDebug()<<s;
    query.prepare(s);
    for (int i = 0; i < r.size(); ++i) {
        query.addBindValue(r.at(i));
    }
}

bool DatabaseRequest::recordUpdateQuery(QSqlQuery &query,
                                        QVariantMap values,
                                        const QString &table,
                                        const QString &tail) const
{
    values = filterFields(table, filterNullValues(values));
    if (values.isEmpty())
        return false;
    QString s = QString("UPDATE %1 SET %2=? %3").arg(table).arg(values.keys().join("=?,")).arg(tail);
    //qDebug()<<s;
    query.prepare(s);
    for (auto v : values.values()) {
        query.addBindValue(v);
    }
    return true;
}
bool DatabaseRequest::recordInsertQuery(QSqlQuery &query,
                                        QVariantMap values,
                                        const QString &table,
                                        const QString &tail) const
{
    values = filterFields(table, filterNullValues(values));
    if (values.isEmpty())
        return false;
    QStringList vlist;
    const QStringList &keys = values.keys();
    for (int i = 0; i < keys.size(); ++i)
        vlist.append("?");
    QString s = QString("INSERT INTO %1(%2) VALUES(%3) %4")
                    .arg(table)
                    .arg(keys.join(","))
                    .arg(vlist.join(','))
                    .arg(tail);
    //qDebug()<<s;
    query.prepare(s);
    for (int i = 0; i < keys.size(); ++i) {
        query.addBindValue(values.value(keys.at(i)));
    }
    return true;
}

QVariantMap DatabaseRequest::filterNullValues(QVariantMap values)
{
    foreach (QString key, values.keys()) {
        const QVariant &v = values.value(key);
        if (v.isNull())
            values.remove(key);
        else if (v.toString().isEmpty())
            values.remove(key);
    }
    return values;
}
QVariantMap DatabaseRequest::filterIdValues(QVariantMap values)
{
    foreach (QString key, values.keys()) {
        if ((key == "key" || key.endsWith("ID")) && (!(key.endsWith("uid", Qt::CaseInsensitive)))) {
            values.remove(key);
        }
    }
    return filterNullValues(values);
}
QVariantMap DatabaseRequest::filterFields(QString tableName, QVariantMap values) const
{
    QStringList fields = db->tableFields(tableName);
    for (auto k : values.keys()) {
        if (fields.contains(k))
            continue;
        values.remove(k);
    }
    return values;
}

QStringList DatabaseRequest::fieldNames(QSqlQuery &query)
{
    QStringList st;
    const QSqlRecord &r = query.record();
    for (int i = 0; i < r.count(); ++i) {
        st.append(r.fieldName(i));
    }
    return st;
}
QVariantList DatabaseRequest::values(QSqlQuery &query, const QStringList &names)
{
    QVariantList v;
    for (auto const &n : names) {
        v.append(query.value(n));
    }
    return v;
}

DatabaseRequest::Records DatabaseRequest::queryRecords(QSqlQuery &query) const
{
    Records records;
    if (query.next()) {
        records.names = fieldNames(query);
        do {
            if (const_cast<DatabaseRequest *>(this)->discarded())
                return Records();
            records.values.append(values(query, records.names));
        } while (query.next());
    }
    return records;
}
QVariantMap DatabaseRequest::queryRecord(QSqlQuery &query, QVariantMap info) const
{
    foreach (QString s, fieldNames(query)) {
        if (!info.value(s).toString().isEmpty())
            continue;
        info[s] = query.value(s);
    }
    return info;
}
QVariantMap DatabaseRequest::queryRecord(Records &records, QVariantMap info, int i)
{
    if (records.values.isEmpty())
        return info;
    foreach (QString s, records.names) {
        if (info.contains(s))
            continue;
        info[s] = records.values.at(i).at(records.names.indexOf(s));
    }
    return info;
}

void DatabaseRequest::getHash(QCryptographicHash &h, const Records &records) const
{
    for (int i = 0; i < records.names.size(); ++i) {
        h.addData(records.names.at(i).toUtf8());
    }
    for (int i = 0; i < records.values.size(); ++i) {
        const QVariantList &r = records.values.at(i);
        for (int j = 0; j < r.size(); ++j) {
            h.addData(r.at(j).toString().toUtf8());
        }
    }
}
void DatabaseRequest::getHash(QCryptographicHash &h, const QVariantMap &map) const
{
    foreach (QString s, map.keys()) {
        h.addData(s.toUtf8());
        const QVariant &v = map.value(s);
        if (v.typeId() == QMetaType::QVariantList) {
            for (auto const &i : v.value<QVariantList>())
                h.addData(i.toString().toUtf8());
        } else {
            h.addData(v.toString().toUtf8());
        }
    }
}
void DatabaseRequest::getHash(QCryptographicHash &h, QSqlQuery &query) const
{
    if (!query.next())
        return;
    QStringList names = fieldNames(query);
    for (int i = 0; i < names.size(); ++i) {
        h.addData(names.at(i).toUtf8());
    }
    do {
        for (int i = 0; i < names.size(); ++i) {
            h.addData(query.value(i).toString().toUtf8());
        }
    } while (query.next());
}
