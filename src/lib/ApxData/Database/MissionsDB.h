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
#ifndef MissionsDB_H
#define MissionsDB_H
//=============================================================================
#include <Database/DatabaseSession.h>
#include <Protocols/ProtocolMission.h>
#include <QtCore>
//=============================================================================
class MissionsDB : public DatabaseSession
{
    Q_OBJECT
public:
    explicit MissionsDB(QObject *parent, QString sessionName);
};
//=============================================================================
class DBReqMissions : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqMissions();
};
//=============================================================================
//=============================================================================
class DBReqMissionsUpdateDetails : public DBReqMissions
{
    Q_OBJECT
public:
    explicit DBReqMissionsUpdateDetails(quint64 missionID, QVariantMap details)
        : DBReqMissions()
        , missionID(missionID)
        , details(details)
    {}
    bool run(QSqlQuery &query);

protected:
    quint64 missionID;
    QVariantMap details;
};
//=============================================================================
class DBReqMissionsFindSite : public DBReqMissions
{
    Q_OBJECT
public:
    explicit DBReqMissionsFindSite(double lat, double lon)
        : DBReqMissions()
        , siteID(0)
        , lat(lat)
        , lon(lon)
    {}
    bool run(QSqlQuery &query);
    //result
    quint64 siteID;
    QString site;

private:
    double lat;
    double lon;
signals:
    void siteFound(quint64 siteID, QString site);
};
class DBReqMissionsSaveSite : public DBReqMissions
{
    Q_OBJECT
public:
    explicit DBReqMissionsSaveSite(QString title, double lat, double lon, quint64 key = 0)
        : DBReqMissions()
        , title(title)
        , lat(lat)
        , lon(lon)
        , key(key)
    {}

private:
    QString title;
    double lat;
    double lon;
    quint64 key;

protected:
    bool run(QSqlQuery &query);
signals:
    void siteAdded(QString title);
    void siteModified(QString title);
};
class DBReqMissionsRemoveSite : public DBReqMissions
{
    Q_OBJECT
public:
    explicit DBReqMissionsRemoveSite(quint64 key)
        : DBReqMissions()
        , key(key)
    {}

private:
    quint64 key;

protected:
    bool run(QSqlQuery &query);
signals:
    void siteRemoved();
};
//=============================================================================
//=============================================================================
class DBReqMissionsSave : public DBReqMissionsUpdateDetails
{
    Q_OBJECT
public:
    explicit DBReqMissionsSave(ProtocolMission::Mission mission, QVariantMap details, quint64 t = 0)
        : DBReqMissionsUpdateDetails(0, details)
        , t(t ? t : QDateTime::currentDateTime().toMSecsSinceEpoch())
        , reqSite(mission.lat, mission.lon)
    {
        makeRecords(mission);
        connect(&reqSite, &DBReqMissionsFindSite::siteFound, this, &DBReqMissionsSave::siteFound);
    }
    bool run(QSqlQuery &query);
    QVariantMap info;

private:
    quint64 t;
    DatabaseRequest::Records rw, wp, tw, pi;
    void makeRecords(const ProtocolMission::Mission &mission);
    void makeRecords(const QList<ProtocolMission::Item> &items, DatabaseRequest::Records &records);
    DBReqMissionsFindSite reqSite;
signals:
    void missionHash(QString hash);
    void siteFound(quint64 siteID, QString site);
};
//=============================================================================
//=============================================================================
class DBReqMissionsLoad : public DBReqMissions
{
    Q_OBJECT
public:
    explicit DBReqMissionsLoad(QString hash)
        : DBReqMissions()
        , hash(hash)
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap info;
    QVariantMap details;
    ProtocolMission::Mission mission;

private:
    QString hash;
    void readItems(QSqlQuery &query, QList<ProtocolMission::Item> &items);

signals:
    void loaded(QVariantMap info, QVariantMap details, ProtocolMission::Mission data);
};
//=============================================================================
#endif
