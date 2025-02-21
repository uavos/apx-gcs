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

class Unit;
class UnitMission;
class MissionTools;

class ElevationMap : public Fact
{
    Q_OBJECT
    Q_PROPERTY(double elevation READ elevation WRITE setElevation NOTIFY elevationChanged)
    // Q_PROPERTY(QGeoCoordinate clickCoordinate READ clickCoordinate WRITE setClickCoordinate NOTIFY
    //                clickCoordinateChanged)

public:
    explicit ElevationMap(Fact *parent = nullptr);

    Fact *f_use;
    Fact *f_path;
    Fact *f_util;
    Fact *f_control;

    Unit *unit() const;
    UnitMission *mission() const;
    MissionTools *missionTools() const;
    Fact *aglset() const;

    double elevation() const;
    void setElevation(double v);
    // QGeoCoordinate clickCoordinate() const;
    // void setClickCoordinate(const QGeoCoordinate &v);

    Q_INVOKABLE void setElevationByCoordinate(const QGeoCoordinate &v);
    Q_INVOKABLE double getElevationByCoordinate(const QGeoCoordinate &v);

protected:
    double m_elevation;
    // QGeoCoordinate m_clickCoordinate;

private:
    std::shared_ptr<AbstractElevationDB> m_elevationDB;

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
    void elevationChanged();
};
