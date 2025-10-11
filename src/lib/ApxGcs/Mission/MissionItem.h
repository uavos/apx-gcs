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

#include "MissionGroup.h"
#include "MissionPoint.h"
#include "UnitMission.h"

#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoRectangle>
#include <QtCore>

#include <XbusMission.h>

class MissionItem : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

    Q_PROPERTY(QGeoPath geoPath READ geoPath NOTIFY geoPathChanged)
    Q_PROPERTY(double bearing READ bearing NOTIFY bearingChanged)
    Q_PROPERTY(uint time READ time NOTIFY timeChanged)
    Q_PROPERTY(uint distance READ distance NOTIFY distanceChanged)

    Q_PROPERTY(uint totalDistance READ totalDistance NOTIFY totalDistanceChanged)
    Q_PROPERTY(uint totalTime READ totalTime NOTIFY totalTimeChanged)

    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged)

public:
    explicit MissionItem(MissionGroup *parent,
                         const QString &name,
                         const QString &title,
                         const QString &descr);

    MissionGroup *group;

    Fact *f_order;
    MissionPoint *f_pos;

    Fact *f_remove;

    Q_INVOKABLE virtual QGeoRectangle boundingGeoRectangle() const;

    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

public slots:
    void updatePath();
    void resetPath();

protected:
    virtual QGeoPath getPath();

    virtual void hashData(QCryptographicHash *h) const override;

    MissionItem *prevItem() const;
    MissionItem *nextItem() const;

    auto unit() const { return group->mission->unit; }

    bool blockUpdates{};
    bool blockUpdateCoordinate{};

private slots:
    virtual void updateTitle();
    virtual void updateStatus();
    virtual void updateCoordinate();
    virtual void updateOrder();

    void updateOrderState();
    void updateSelected();

    void selectTriggered();

signals:
    void itemDataLoaded();

    //---------------------------------------
    // PROPERTIES
public:
    QGeoCoordinate coordinate() const;
    void setCoordinate(const QGeoCoordinate &v);

    QGeoPath geoPath() const;
    void setGeoPath(const QGeoPath &v);

    double bearing() const;
    void setBearing(const double &v);

    uint time() const; //travelled time to current wp from previous [s]
    void setTime(uint v);

    uint distance() const; //travelled distance to current wp from previous [m]
    void setDistance(uint v);

    uint totalDistance() const; //estimated total travel distance [m]
    void setTotalDistance(uint v);

    uint totalTime() const; //estimated total time of arrival [s]
    void setTotalTime(uint v);

    bool selected() const;
    void setSelected(bool v);

protected:
    QGeoCoordinate m_coordinate;
    QGeoPath m_geoPath;
    double m_bearing{};
    uint m_time{};
    uint m_distance{};

    uint m_totalDistance{};
    uint m_totalTime{};

    bool m_selected{};

signals:
    void coordinateChanged();
    void geoPathChanged();
    void bearingChanged();
    void timeChanged();
    void distanceChanged();

    void totalDistanceChanged();
    void totalTimeChanged();

    void selectedChanged();
};
