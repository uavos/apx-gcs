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
