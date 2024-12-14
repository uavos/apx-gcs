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
    if (db) {
        connect(this,
                &DatabaseRequest::dbModified,
                db,
                &DatabaseSession::modifiedNotify,
                Qt::QueuedConnection);
    }
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
    else {
        emit success();
        emit finished(Success);
    }
}

bool DatabaseRequest::run(QSqlQuery &query)
{
    // default request implementation
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

    // emit signal with records
    QJsonArray records;
    while (query.next()) {
        auto jso = record_to_json(query.record(), {});
        if (jso.isEmpty())
            continue;
        records.append(jso);
    }
    if (!records.isEmpty())
        emit queryResults(records);

    return true;
}

void DatabaseRequest::discard()
{
    if (m_discarded)
        return;
    m_discarded = true;
    emit discardRequested();
}

bool DatabaseRequest::discarded()
{
    return m_discarded;
}

bool DatabaseRequest::recordUpdateQuery(QSqlQuery &query,
                                        const QJsonObject &jso,
                                        const QString &table,
                                        const QString &tail) const
{
    const auto keys = jso.keys();
    auto s = QString("UPDATE %1 SET %2=? %3").arg(table).arg(keys.join("=?,")).arg(tail);
    //qDebug()<<s;
    query.prepare(s);
    for (auto key : keys) {
        const auto jsv = jso.value(key);
        if (jsv.isObject())
            query.addBindValue(QJsonDocument(jsv.toObject()).toJson(QJsonDocument::Compact));
        else if (jsv.isArray())
            query.addBindValue(QJsonDocument(jsv.toArray()).toJson(QJsonDocument::Compact));
        else
            query.addBindValue(jso.value(key).toVariant());
    }
    return true;
}

QJsonObject DatabaseRequest::record_to_json(const QSqlRecord &record, const QStringList &names)
{
    QJsonObject jso;
    for (auto i = 0; i < record.count(); ++i) {
        auto k = record.fieldName(i);
        if (!names.isEmpty() && !names.contains(k))
            continue;
        if (jso.contains(k))
            continue;
        const auto v = record.value(i);
        if (v.isNull())
            continue;
        jso[k] = QJsonValue::fromVariant(v);
    }
    return jso;
}

void DatabaseRequest::getHash(QCryptographicHash &h, const QJsonValue &jsv) const
{
    if (jsv.isObject()) {
        auto jso = jsv.toObject();
        for (auto i = jso.begin(); i != jso.end(); ++i) {
            h.addData(i.key().toUtf8());
            getHash(h, i.value());
        }
    } else if (jsv.isArray()) {
        auto jsa = jsv.toArray();
        for (auto i = 0; i < jsa.size(); ++i) {
            getHash(h, jsa.at(i));
        }
    } else {
        h.addData(jsv.toVariant().toString().toUtf8());
    }
}

void DatabaseRequest::getHash(QCryptographicHash &h, QSqlQuery &query) const
{
    if (!query.next())
        return;
    const auto &r = query.record();
    for (int i = 0; i < r.count(); ++i) {
        h.addData(r.fieldName(i).toUtf8());
    }
    do {
        for (int i = 0; i < r.count(); ++i) {
            h.addData(r.value(i).toString().toUtf8());
        }
    } while (query.next());
}
