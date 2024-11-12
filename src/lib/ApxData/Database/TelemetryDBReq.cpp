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
#include "TelemetryDBReq.h"

#include <App/AppDirs.h>
#include <App/AppRoot.h>

DBReqTelemetry::DBReqTelemetry()
    : DatabaseRequest(Database::instance()->telemetry)
{}
bool DBReqTelemetry::run(QSqlQuery &query)
{
    Q_UNUSED(query)
    return true;
}

bool DBReqTelemetryUpdateMandala::run(QSqlQuery &query)
{
    // called by LOCAL vehicle with 'records' initialized to current mandala fields
    connect(
        this,
        &DBReqTelemetryUpdateMandala::progress,
        db,
        [this](int v) { db->setProgress(v); },
        Qt::QueuedConnection);

    const QStringList &n = records.names;
    int i_id = n.indexOf("id");
    int i_name = n.indexOf("name");

    TelemetryDB::FieldsByUID byUID;
    TelemetryDB::FieldsByName byName;
    QList<quint64> all_keys;

    //load existing fields
    query.prepare("SELECT * FROM TelemetryFields");
    if (!query.exec())
        return false;
    Records db_records = queryRecords(query);
    const QStringList &rn = db_records.names;
    int i_r_name = rn.indexOf("name");

    bool mod = false;
    for (auto const &f : records.values) {
        QString f_name = f.at(i_name).toString();
        mandala::uid_t uid = f.at(i_id).toUInt();

        quint64 key = 0;
        bool upd = false;
        for (auto const &r : db_records.values) {
            const QString &name = r.at(i_r_name).toString();
            if (name == f_name) {
                key = r.at(0).toULongLong();
                byUID.insert(uid, key);
                byName.insert(f_name, key);
                all_keys.append(key);
                //check entire row match
                for (int i = 1; i < r.size(); ++i) {
                    if (r.at(i).toString() == f.value(n.indexOf(rn.at(i))).toString()) {
                        continue;
                    }
                    //qWarning() << r.at(i) << f.value(n.indexOf(rn.at(i)));
                    upd = true; //record update needed
                    break;
                }
                break; //exsisting record found
            }
        }
        if (key && !upd)
            continue;
        if (!mod) {
            if (!db->transaction(query))
                return false;
            mod = true;
        }
        if (!key) {
            apxConsole() << "new telemetry field:" << f_name;
            query.prepare("INSERT INTO TelemetryFields(name) VALUES(?)");
            query.addBindValue(f_name);
            if (!query.exec())
                return false;
            key = query.lastInsertId().toULongLong();
            byUID.insert(uid, key);
            byName.insert(f_name, key);
            all_keys.append(key);
        } else {
            apxConsole() << "update telemetry field:" << f_name;
        }
        //update existing record
        QStringList nlist = rn;
        if (!nlist.isEmpty()) {
            nlist.removeAt(0); //skip key
        }
        if (nlist.isEmpty()) {
            nlist = n;
            nlist.removeOne("alias");
        }
        QString qs = QString("UPDATE TelemetryFields SET %1=? WHERE key=?").arg(nlist.join("=?,"));
        query.prepare(qs);
        for (auto s : nlist) {
            query.addBindValue(f.value(n.indexOf(s)));
        }
        query.addBindValue(key);
        if (!query.exec())
            return false;
    }
    if (mod) {
        if (!db->commit(query))
            return false;
        apxMsgW() << tr("Telemetry DB updated");
    }

    //check for deprecated records
    QList<quint64> rmlist;
    for (auto const &r : db_records.values) {
        quint64 key = r.at(0).toULongLong();
        if (all_keys.contains(key))
            continue;
        apxConsole() << "remove telemetry field:" << r.at(i_r_name).toString();
        rmlist.append(key);
    }
    if (!rmlist.isEmpty()) {
        db->disable();
        emit progress(0);
        apxMsgW() << tr("Telemetry DB maintenance in progress...");
        QStringList st;
        for (auto k : rmlist)
            st << QString::number(k);
        if (!query.exec("PRAGMA foreign_keys = OFF"))
            return false;
        query.prepare(
            QString("DELETE FROM TelemetryUplink WHERE fieldID IN (%1)").arg(st.join(',')));
        if (!query.exec())
            return false;
        query.prepare(
            QString("DELETE FROM TelemetryDownlink WHERE fieldID IN (%1)").arg(st.join(',')));
        if (!query.exec())
            return false;
        query.prepare(QString("DELETE FROM TelemetryFields WHERE key IN (%1)").arg(st.join(',')));
        if (!query.exec())
            return false;
        if (!query.exec("PRAGMA foreign_keys = ON"))
            return false;
        emit progress(-1);
        apxMsgW() << tr("Telemetry DB ready");
        db->enable();
    }

    static_cast<TelemetryDB *>(db)->updateFieldsMap(byUID, byName);

    return true;
}

bool DBReqTelemetryEmptyTrash::run(QSqlQuery &query)
{
    db->disable();
    emit progress(0);
    bool rv = false;
    do {
        query.prepare("SELECT COUNT(*) FROM Telemetry WHERE trash IS NOT NULL");
        if (!query.exec())
            break;
        if (!query.next())
            break;

        qint64 cnt = query.value(0).toLongLong();
        qint64 dcnt = 0;
        if (cnt > 0) {
            apxMsg() << tr("Permanently deleting %1 records").arg(cnt).append("...");

            while (dcnt < cnt) {
                query.prepare(
                    "SELECT key FROM Telemetry WHERE trash IS NOT NULL ORDER BY time DESC LIMIT 1");
                if (!query.exec())
                    break;
                if (!query.next())
                    break;
                quint64 key = query.value(0).toULongLong();

                if (!db->transaction(query))
                    break;
                query.prepare("DELETE FROM Telemetry WHERE key=?");
                query.addBindValue(key);
                if (!query.exec())
                    break;
                if (!db->commit(query))
                    break;

                dcnt++;
                emit progress((dcnt * 100 / cnt));
                if (discarded())
                    break;
            }
        }
        if (dcnt < cnt)
            apxMsgW() << tr("Telemetry trash not empty") << cnt - dcnt;
        else {
            apxMsg() << tr("Telemetry trash is empty");
            rv = true;
        }
    } while (0);
    emit progress(-1);
    db->enable();
    return rv;
}

bool DBReqTelemetryEmptyCache::run(QSqlQuery &query)
{
    db->disable();
    emit progress(0);
    bool rv = false;
    do {
        query.prepare("SELECT COUNT(*) FROM TelemetryCache");
        if (!query.exec())
            break;
        if (!query.next())
            break;

        qint64 cnt = query.value(0).toLongLong();
        qint64 dcnt = 0;
        if (cnt > 0) {
            apxMsg() << tr("Deleting %1 cached records").arg(cnt).append("...");

            while (dcnt < cnt) {
                query.prepare("SELECT key FROM TelemetryCache ORDER BY time DESC LIMIT 1");
                if (!query.exec())
                    break;
                if (!query.next())
                    break;
                quint64 key = query.value(0).toULongLong();

                if (!db->transaction(query))
                    break;
                query.prepare("DELETE FROM TelemetryCache WHERE key=?");
                query.addBindValue(key);
                if (!query.exec())
                    break;
                if (!db->commit(query))
                    break;

                dcnt++;
                emit progress((dcnt * 100 / cnt));
                if (discarded())
                    break;
            }
        }
        if (dcnt < cnt)
            apxMsgW() << tr("Telemetry cache not empty") << cnt - dcnt;
        else {
            apxMsg() << tr("Telemetry cache is empty");
            rv = true;
        }
    } while (0);
    emit progress(-1);
    db->enable();
    return rv;
}

bool DBReqTelemetryStats::run(QSqlQuery &query)
{
    query.prepare("SELECT COUNT(*) FROM Telemetry");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 cntTotal = query.value(0).toULongLong();

    query.prepare("SELECT COUNT(*) FROM Telemetry WHERE trash IS NOT NULL");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 cntTrash = query.value(0).toULongLong();

    apxMsg() << tr("Telemetry records").append(":") << cntTotal << tr("total").append(",")
             << cntTrash << tr("trash");

    emit totals(cntTotal, cntTrash);
    return true;
}

bool DBReqTelemetryRecover::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM Telemetry WHERE hash=?");
    query.addBindValue(_hash);
    if (!query.exec())
        return false;
    if (!query.next()) {
        emit unavailable(_hash);
        return true;
    }
    auto telemetryID = query.value(0).toULongLong();
    auto trash = query.value("trash").toBool();
    if (trash) {
        query.prepare("UPDATE Telemetry SET trash=NULL WHERE key=?");
        query.addBindValue(telemetryID);
        if (!query.exec())
            return false;
    }
    emit available(telemetryID, _hash);
    return true;
}

bool DBReqTelemetryCreateRecord::run(QSqlQuery &query)
{
    // qDebug() << vehicleUID << callsign << t;

    query.prepare("INSERT INTO Telemetry(time, file, vehicleUID, callsign, comment, trash) "
                  "VALUES(?, ?, ?, ?, ?, ?)");
    const auto &info = _info;
    query.addBindValue(_t);
    query.addBindValue(_fileName);
    query.addBindValue(info["vehicle"]["uid"].toVariant());
    query.addBindValue(info["vehicle"]["callsign"].toVariant());
    query.addBindValue(info["conf"].toVariant());
    query.addBindValue(_trash ? 1 : QVariant());
    if (!query.exec())
        return false;
    _telemetryID = query.lastInsertId().toULongLong();

    emit recordCreated(_telemetryID);
    emit dbModified();
    //qDebug() << telemetryID;
    return true;
}

bool DBReqTelemetryModelRecordsList::run(QSqlQuery &query)
{
    QString s = "SELECT key, time FROM Telemetry";
    if (!_filter.isEmpty())
        s += " WHERE " + _filter;
    s += " ORDER BY time DESC";

    query.prepare(s);
    if (!query.exec())
        return false;

    DatabaseModel::RecordsList records;

    while (query.next()) {
        records.append(query.value(0).toULongLong());
    }

    emit recordsList(records);

    return true;
}

bool DBReqTelemetryLoadInfo::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM Telemetry WHERE key=?");
    query.addBindValue(_id);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    auto r = query.record();

    // prepare info for models
    QJsonObject j;

    QString stime = QDateTime::fromMSecsSinceEpoch(r.value("time").toLongLong())
                        .toString("yyyy MMM dd hh:mm:ss");
    QString callsign = r.value("callsign").toString();
    QString comment = r.value("comment").toString();
    QString notes = r.value("notes").toString();
    QString total;
    quint64 t = r.value("duration").toULongLong();
    if (t > 0)
        total = AppRoot::timeToString(t / 1000);
    QStringList descr;
    if (r.value("trash").toBool())
        descr << tr("deleted").toUpper();

    if (!comment.isEmpty())
        descr << comment;
    if (!notes.isEmpty())
        descr << notes;
    QStringList value;
    if (!callsign.isEmpty())
        value << callsign;
    if (!total.isEmpty())
        value << total;

    j["title"] = stime;
    j["descr"] = descr.join(" - ");
    j["value"] = value.join(' ');

    emit modelInfo(_id, j);

    // provide parsed file info JSON object
    QJsonObject info = QJsonDocument::fromJson(r.value("info").toByteArray()).object();
    emit recordInfo(_id, info, notes);

    return true;
}

bool DBReqTelemetryModelTrash::run(QSqlQuery &query)
{
    // toggle trash flag
    query.prepare("SELECT * FROM Telemetry WHERE key=?");
    query.addBindValue(_id);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    auto trash = !query.value("trash").toBool();
    if (trash) {
        qDebug() << "Moving record to trash" << _id << query.value("file").toString();
    } else {
        qDebug() << "Restoring record from trash" << _id << query.value("file").toString();
    }

    query.prepare("UPDATE Telemetry SET trash=? WHERE key=?");
    query.addBindValue(trash ? 1 : QVariant());
    query.addBindValue(_id);
    if (!query.exec())
        return false;

    emit dbModified();

    return true;
}

bool DBReqTelemetryLoadFile::run(QSqlQuery &query)
{
    //check for invalid cache (telemetry hash is null)
    query.prepare("SELECT * FROM Telemetry WHERE key=?");
    query.addBindValue(_id);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    const auto file = query.value("file").toString();
    if (file.isEmpty()) {
        qWarning() << "empty file name";
        // TODO convert DB data to file
        return false;
    }

    if (query.value("trash").toBool()) {
        qDebug() << "Recovering record from trash";
        auto req = new DBReqTelemetryModelTrash(_id, false);
        req->exec(); // chain next
    }

    const auto hash = query.value("hash").toString();

    auto filePath = AppDirs::storage().absoluteFilePath(telemetry::APXTLM_FTYPE);
    filePath = QDir(filePath).absoluteFilePath(file + '.' + telemetry::APXTLM_FTYPE);

    if (!_reader.open(filePath))
        return false;

    if (!_reader.parse_payload())
        return false;

    bool still_writing = _reader.is_still_writing();
    if (still_writing) {
        qDebug() << "File is still writing";
    }

    // update db record when needed
    do {
        const auto &rinfo = _reader.info();

        QVariantMap q;
        if (!still_writing) {
            // hash only set on finished file
            auto new_hash = rinfo["hash"].toString();
            if (new_hash.isEmpty()) {
                qWarning() << "empty hash";
                break;
            }
            if (hash == new_hash) // no changes
                break;
            q["hash"] = new_hash;
        }

        q["duration"] = rinfo["duration"].toInteger();
        q["info"] = QJsonDocument(rinfo).toJson(QJsonDocument::Compact);

        // write changes to db
        if (recordUpdateQuery(query, q, "Telemetry", "WHERE key=?")) {
            query.addBindValue(_id);
            if (!query.exec())
                return false;

            qDebug() << "Record hash updated";

            emit dbModified();
        }
    } while (0);

    // read record info
    return DBReqTelemetryLoadInfo::run(query);
}
