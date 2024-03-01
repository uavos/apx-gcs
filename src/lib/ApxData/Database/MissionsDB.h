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

#include <Database/DatabaseSession.h>
#include <QtCore>

class MissionsDB : public DatabaseSession
{
    Q_OBJECT
public:
    explicit MissionsDB(QObject *parent, QString sessionName);
};

class DBReqMissions : public DatabaseRequest
{
    Q_OBJECT
public:
    explicit DBReqMissions();
};

class DBReqMissionsLoad : public DBReqMissions
{
    Q_OBJECT
public:
    explicit DBReqMissionsLoad(QString hash)
        : DBReqMissions()
        , hash(hash)
    {}
    bool run(QSqlQuery &query);

    auto mission() const { return _mission; }

private:
    QString hash;
    QVariantMap _mission;

    QVariant readItems(QSqlQuery &query);

signals:
    void loaded(QVariant data);
};

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

class DBReqMissionsSave : public DBReqMissions
{
    Q_OBJECT
public:
    explicit DBReqMissionsSave(QVariant var)
        : DBReqMissions()
        , _data(var.value<QVariantMap>())
        , _reqSite(_data.value("lat").toDouble(), _data.value("lon").toDouble())
    {
        connect(&_reqSite, &DBReqMissionsFindSite::siteFound, this, &DBReqMissionsSave::siteFound);
    }
    bool run(QSqlQuery &query);

private:
    quint64 _missionID;
    QVariantMap _data;
    DBReqMissionsFindSite _reqSite;

    bool writeItems(QSqlQuery &query, const QVariant &var, QString tableName);

signals:
    void missionHash(QString hash);
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

protected:
    bool run(QSqlQuery &query);

private:
    QString title;
    double lat;
    double lon;
    quint64 key;

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

protected:
    bool run(QSqlQuery &query);

private:
    quint64 key;

signals:
    void siteRemoved();
};
