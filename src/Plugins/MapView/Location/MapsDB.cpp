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
#include "MapsDB.h"
#include "TileLoader.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
//=============================================================================
MapsDB::MapsDB(QObject *parent, QString sessionName)
    : DatabaseSession(parent, AppDirs::db().absoluteFilePath("maps.db"), sessionName)
{
    new DBReqMakeTable(this,
                       "TileProviders",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "name TEXT NOT NULL"
                                     << "descr TEXT"
                                     << "format TEXT" //data format
    );

    new DBReqMakeTable(this,
                       "Tiles",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "providerID INTEGER NOT NULL"
                                     << "hash INTEGER NOT NULL"
                                     << "tile BLOB"
                                     << "version INTEGER DEFAULT 0"
                                     << "size INTEGER"
                                     << "time INTEGER DEFAULT 0" //time downloaded
                                     << "FOREIGN KEY(providerID) REFERENCES TileProviders(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "Tiles", "hash", false);
    new DBReqMakeIndex(this, "Tiles", "providerID,hash", true);

    DBReqTileProviders *reqProviders = new DBReqTileProviders(this);
    connect(
        reqProviders,
        &DBReqTileProviders::providersLoaded,
        this,
        [this](QHash<quint8, quint64> v) { providersMap.swap(v); },
        Qt::QueuedConnection);
    reqProviders->exec();
}
//=============================================================================
//=============================================================================
bool DBReqTileProviders::run(QSqlQuery &query)
{
    QHash<quint8, quint64> providersMap;
    //sync providers
    query.prepare("SELECT * FROM TileProviders");
    if (!query.exec())
        return false;

    QMetaEnum metaEnum = QMetaEnum::fromType<TileLoader::MapID>();
    QStringList pnames;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        pnames << metaEnum.key(i);
    }
    while (query.next()) {
        QString s = query.value(1).toString();
        if (!pnames.contains(s)) {
            apxConsoleW() << "deprecated tile provider maps DB" << s;
            continue;
        }
        pnames.removeOne(s);
        providersMap.insert(static_cast<TileLoader::MapID>(metaEnum.keyToValue(s.toUtf8().data())),
                            query.value(0).toULongLong());
    }
    //qDebug()<<pnames;
    foreach (QString s, pnames) {
        query.prepare("INSERT INTO TileProviders(name) VALUES(?)");
        query.addBindValue(s);
        if (!query.exec())
            return false;
        providersMap.insert(static_cast<TileLoader::MapID>(metaEnum.keyToValue(s.toUtf8().data())),
                            query.lastInsertId().toULongLong());
        //qDebug()<<"add provider maps DB"<<s;
    }
    emit providersLoaded(providersMap);
    return true;
}
//=============================================================================
bool DBReqLoadTile::run(QSqlQuery &query)
{
    connect(this, &DBReqLoadTile::tileLoaded, db, &MapsDB::tileLoaded, Qt::QueuedConnection);
    connect(this, &DBReqLoadTile::tileNotExists, db, &MapsDB::tileNotExists, Qt::QueuedConnection);

    query.prepare("SELECT tile FROM Tiles WHERE providerID = ? AND hash = ?");
    query.addBindValue(mapID);
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        emit tileLoaded(uid, query.value(0).toByteArray());
    } else {
        emit tileNotExists(uid);
    }
    return true;
}
//=============================================================================
bool DBReqSaveTile::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM Tiles WHERE providerID = ? AND hash = ?");
    query.addBindValue(mapID);
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        quint64 key = query.value(0).toULongLong();
        query.prepare("UPDATE Tiles"
                      " SET tile=?, version=?, size=?, time=?"
                      " WHERE key=?");
        query.addBindValue(tile);
        query.addBindValue(version);
        query.addBindValue(tile.size());
        query.addBindValue(t);
        query.addBindValue(key);
    } else {
        query.prepare("INSERT INTO Tiles"
                      " (providerID, hash, tile, version, size, time)"
                      " VALUES(?, ?, ?, ?, ?, ?)");
        query.addBindValue(mapID);
        query.addBindValue(hash);
        query.addBindValue(tile);
        query.addBindValue(version);
        query.addBindValue(tile.size());
        query.addBindValue(t);
    }
    return query.exec();
}
//=============================================================================
