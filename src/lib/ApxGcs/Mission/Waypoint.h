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
#include "WaypointActions.h"
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QtCore>

class Waypoint : public MissionItem
{
    Q_OBJECT

    Q_PROPERTY(bool reachable READ reachable WRITE setReachable NOTIFY reachableChanged)
    Q_PROPERTY(bool warning READ warning WRITE setWarning NOTIFY warningChanged)

public:
    explicit Waypoint(MissionGroup *parent);

    Fact *f_altitude;
    Fact *f_amsl;

    Fact *f_atrack;
    Fact *f_xtrack;

    WaypointActions *f_actions;

    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

protected:
    QGeoPath getPath() override;

private:
    QString _altUnits;

private slots:
    void updateTitle() override;
    void updateDescr();
    void updateAMSL();

    //---------------------------------------
    // PROPERTIES
public:
    bool reachable() const;
    void setReachable(bool v);

    bool warning() const;
    void setWarning(bool v);

protected:
    bool m_reachable{};
    bool m_warning{};

signals:
    void reachableChanged();
    void warningChanged();
};
