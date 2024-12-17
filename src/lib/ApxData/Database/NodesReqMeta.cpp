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
#include "NodesReqMeta.h"

using namespace db::nodes;

bool SaveFieldMeta::run(QSqlQuery &query)
{
    uint tcnt = 0, ncnt = 0, ucnt = 0;
    for (auto it = _jso.begin(); it != _jso.end(); ++it) {
        auto name = it.key();
        auto jso = it.value().toObject();

        auto version = QVersionNumber::fromString(jso.value("version").toString());
        jso.remove("version");

        if (jso.isEmpty())
            continue;

        const auto meta = QJsonDocument(jso).toJson(QJsonDocument::Compact);

        // qDebug() << name << jso;

        tcnt++;
        query.prepare("SELECT * FROM NodeFieldMeta WHERE name=?");
        query.addBindValue(name);
        if (!query.exec())
            return false;

        if (!query.next()) {
            query.prepare("INSERT INTO NodeFieldMeta(name,version,meta) "
                          "VALUES(?,?,?)");
            query.addBindValue(name);
            query.addBindValue(version.toString());
            query.addBindValue(meta);
            if (!query.exec())
                return false;
            ncnt++;
            continue;
        }

        // update existing
        const auto qversion = QVersionNumber::fromString(query.value("version").toString());
        const auto qmeta = query.value("meta").toString();

        if (version.isNull()) {
            version = qversion;
        } else if (version < qversion) {
            // downgrade not allowed
            continue;
        }

        // check for record update
        if (meta == qmeta) {
            continue; // not changed
        }

        auto key = query.value(0).toULongLong();
        query.prepare("UPDATE NodeFieldMeta SET version=?, meta=? WHERE key=?");
        query.addBindValue(version.toString());
        query.addBindValue(meta);
        query.addBindValue(key);
        if (!query.exec())
            return false;
        ucnt++;
    }

    if (ncnt)
        qDebug() << "new meta:" << ncnt << "of" << tcnt;
    if (ucnt)
        qDebug() << "updated meta:" << ucnt << "of" << tcnt;

    return true;
}
bool LoadFieldMeta::run(QSqlQuery &query)
{
    QJsonObject meta;
    for (auto name : _names) {
        query.prepare("SELECT * FROM NodeFieldMeta WHERE name=?");
        query.addBindValue(name);
        if (!query.exec())
            return false;
        if (!query.next())
            continue;

        auto jso = QJsonDocument::fromJson(query.value("meta").toByteArray()).object();
        meta[name] = jso;
    }

    if (!meta.isEmpty())
        emit dictMetaLoaded(meta);

    return true;
}
