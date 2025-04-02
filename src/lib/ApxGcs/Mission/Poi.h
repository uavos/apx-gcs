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

#include "MissionItem.h"
#include <QtCore>

class Poi : public MissionItem
{
    Q_OBJECT
    Q_PROPERTY(
        QGeoCoordinate radiusPoint READ radiusPoint WRITE setRadiusPoint NOTIFY radiusPointChanged)

public:
    explicit Poi(MissionGroup *parent);

    Fact *f_hmsl;
    Fact *f_radius;
    Fact *f_orbs;
    Fact *f_time;

protected:
    QGeoRectangle boundingGeoRectangle() const;

private slots:
    void updateTitle();
    void updateDescr();

public slots:
    void initElevationMap();

    //---------------------------------------
    // PROPERTIES
public:
    QGeoCoordinate radiusPoint() const;
    void setRadiusPoint(const QGeoCoordinate &v);

protected:
    
signals:
    void radiusPointChanged();
};
