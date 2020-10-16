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
#pragma once

#include "MissionGroup.h"
#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoRectangle>
#include <QtCore>

#include <xbus/XbusMission.h>

class MissionItem : public Fact
{
    Q_OBJECT
    Q_PROPERTY(int missionItemType READ missionItemType CONSTANT)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

    Q_PROPERTY(QGeoPath geoPath READ geoPath NOTIFY geoPathChanged)
    Q_PROPERTY(double course READ course NOTIFY courseChanged)
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
    int missionItemType() const;

    Fact *f_order;
    Fact *f_latitude;
    Fact *f_longitude;

    Fact *f_remove;

    Q_INVOKABLE virtual QGeoRectangle boundingGeoRectangle() const;

public slots:
    void updatePath();
    void resetPath();

protected:
    virtual QGeoPath getPath();

    virtual void hashData(QCryptographicHash *h) const;

    MissionItem *prevItem() const;
    MissionItem *nextItem() const;

private slots:
    virtual void updateTitle();
    virtual void updateStatus();
    virtual void updateCoordinate();
    virtual void updateOrder();

    void updateOrderState();
    void updateSelected();

    void selectTriggered();

private:
    bool blockUpdateCoordinate;

    //---------------------------------------
    // PROPERTIES
public:
    QGeoCoordinate coordinate() const;
    void setCoordinate(const QGeoCoordinate &v);

    QGeoPath geoPath() const;
    void setGeoPath(const QGeoPath &v);

    double course() const;
    void setCourse(const double &v);

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
    double m_course;
    uint m_time;
    uint m_distance;

    uint m_totalDistance;
    uint m_totalTime;

    bool m_selected;

signals:
    void coordinateChanged();
    void geoPathChanged();
    void courseChanged();
    void timeChanged();
    void distanceChanged();

    void totalDistanceChanged();
    void totalTimeChanged();

    void selectedChanged();
};
