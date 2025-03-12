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

#include "ElevationDB.h"
#include <Fact/Fact.h>

#include <memory>

#include <QQmlComponent>
#include <QGeoCoordinate>
#include <QtCore>
#include <QMap>
#include <QSet>

#include <Mission/MissionItem.h>

class Unit;
class UnitMission;
class MissionTools;

class ElevationMap : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

public:
    explicit ElevationMap(Fact *parent = nullptr);

    Fact *f_use;
    Fact *f_path;
    Fact *f_util;
    Fact *f_control{nullptr};

    Unit *unit() const;
    UnitMission *mission() const;
    MissionTools *missionTools() const;
    Fact *aglset() const;

    Q_INVOKABLE void setElevationByCoordinate(const QGeoCoordinate &v);

    QGeoCoordinate coordinate();
    void setCoordinate(const QGeoCoordinate &coordinate);

protected:
    QGeoCoordinate m_coordinate;

private:
    QSharedPointer<AbstractElevationDB> m_elevationDB;
    QMap<QString, int> m_waypoints;
    QSet<QString> m_runways;
    QSet<QString> m_pois;

    void createElevationDatabase();
    void setMissionValues(bool b);
    QObject *qml;

private slots:
    void onOpenTriggered();
    void updateMission();
    void setMissionAgl();
    void getPluginEnableControl();
    void changeExternalsVisibility();
    void updateDBUtility();

signals:
    void coordinateChanged(QGeoCoordinate coordinate);
};
