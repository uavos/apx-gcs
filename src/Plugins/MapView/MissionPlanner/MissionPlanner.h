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
#ifndef MissionPlanner_H
#define MissionPlanner_H
//=============================================================================
#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QGeoShape>
#include <QQmlComponent>
#include <QtCore>
class Vehicle;
class VehicleMission;
//=============================================================================
class MissionPlanner : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate clickCoordinate READ clickCoordinate WRITE setClickCoordinate NOTIFY
                   clickCoordinateChanged)
    Q_PROPERTY(QGeoShape area READ area WRITE setArea NOTIFY areaChanged)

public:
    explicit MissionPlanner(Fact *parent = nullptr);
    ~MissionPlanner();

    Fact *f_add;
    Fact *f_vehicle;

private:
    Vehicle *vehicle() const;
    VehicleMission *mission() const;
    QObject *qml;

    //---------------------------------------
    // PROPERTIES
public:
    QGeoCoordinate clickCoordinate() const;
    void setClickCoordinate(const QGeoCoordinate &v);

    QGeoShape area() const;
    void setArea(const QGeoShape &v);

protected:
    QGeoCoordinate m_clickCoordinate;
    QGeoShape m_area;

signals:
    void clickCoordinateChanged();
    void areaChanged();
};
//=============================================================================
#endif
