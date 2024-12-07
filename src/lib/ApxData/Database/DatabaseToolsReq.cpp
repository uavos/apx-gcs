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
#include "DatabaseToolsReq.h"
#include "DatabaseSession.h"

using namespace db;

bool MakeTable::run(QSqlQuery &query)
{
    query.prepare(QString("PRAGMA table_info('%1')").arg(tableName));
    if (!query.exec())
        return false;

    QStringList fieldsList;
    for (auto i : fields) {
        if (i.contains('('))
            continue;
        if (!i.contains(' '))
            continue;
        fieldsList.append(i.split(' ').first());
    }
    db->updateTableFields(tableName, fieldsList);

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

bool MakeIndex::run(QSqlQuery &query)
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

bool Vacuum::run(QSqlQuery &query)
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

bool Analyze::run(QSqlQuery &query)
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
