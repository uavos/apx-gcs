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
#ifndef MapsDB_H
#define MapsDB_H
//=============================================================================
#include <Database/DatabaseSession.h>
#include <QtCore>
//=============================================================================
class MapsDB : public DatabaseSession
{
    Q_OBJECT

public:
    explicit MapsDB(QObject *parent, QString sessionName);

    QHash<quint8, quint64> providersMap;

signals:
    void tileLoaded(quint64 uid, QByteArray data);
    void tileNotExists(quint64 uid);
};
//=============================================================================
class DBReqTileProviders : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqTileProviders(MapsDB *db)
        : DatabaseRequest(db)
        , mdb(db)
    {}

private:
    MapsDB *mdb;

protected:
    bool run(QSqlQuery &query);
signals:
    void providersLoaded(QHash<quint8, quint64> v);
};
//=============================================================================
class DBReqLoadTile : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqLoadTile(MapsDB *db, quint8 mapPID, quint64 hash, quint64 uid)
        : DatabaseRequest(db)
        , db(db)
        , mapID(db->providersMap.value(mapPID))
        , hash(hash)
        , uid(uid)
    {}

private:
    MapsDB *db;
    quint8 mapID;
    quint64 hash;
    quint64 uid;

protected:
    bool run(QSqlQuery &query);

signals:
    void tileLoaded(quint64 uid, QByteArray data);
    void tileNotExists(quint64 uid);
};
//=============================================================================
class DBReqSaveTile : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqSaveTile(
        MapsDB *db, quint8 mapPID, quint64 hash, quint32 version, const QByteArray &tile)
        : DatabaseRequest(db)
        , mapID(db->providersMap.value(mapPID))
        , hash(hash)
        , version(version)
        , tile(tile)
        , t(QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}

private:
    quint8 mapID;
    quint64 hash;
    quint32 version;
    QByteArray tile;
    qint64 t;

protected:
    bool run(QSqlQuery &query);
};
//=============================================================================
//=============================================================================
#endif
