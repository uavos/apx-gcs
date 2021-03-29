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

#include <ApxMisc/DelayedEvent.h>
#include <Database/DatabaseRequest.h>
#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QtCore>

class VehicleMission;
class Vehicle;
class MissionShare;
class MissionGroup;
class LookupMissions;

class MissionStorage : public QObject
{
    Q_OBJECT
    Q_ENUMS(MissionItemType)

public:
    explicit MissionStorage(VehicleMission *mission);

    VehicleMission *mission;
    QString dbHash;

private:
    DelayedEvent evtUpdateSite;

private slots:
    //database
    void dbSaved(QString hash);
    void dbLoaded(QJsonValue json);
    void dbSiteFound(quint64 siteID, QString site);
    void dbFindSite();

public slots:
    void saveMission();
    void loadMission(QString hash);

signals:
    void loaded();
    void saved();
};
