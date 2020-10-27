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

#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QtCore>
class VehicleMission;
class MissionItem;

class MissionGroup : public Fact
{
    Q_OBJECT
    Q_PROPERTY(uint distance READ distance NOTIFY distanceChanged)
    Q_PROPERTY(uint time READ time NOTIFY timeChanged)
    Q_PROPERTY(QAbstractListModel *mapModel READ mapModel CONSTANT)

public:
    explicit MissionGroup(VehicleMission *parent,
                          const QString &name,
                          const QString &title,
                          const QString &descr,
                          Fact *activeIndex);

    VehicleMission *mission;

    virtual int missionItemType() const { return -1; }

    virtual MissionItem *createObject() { return nullptr; }

    MissionItem *addObject(const QGeoCoordinate &);

    Fact *f_clear;

    Fact *f_activeIndex; //mandala fact to reflect current item index in group

private:
    QTimer updateTimeTimer;
    QTimer updateDistanceTimer;
    QString _descr;

protected:
    void objectAdded(Fact *fact);

private slots:
    void updateTimeDo();
    void updateDistanceDo();
    void updateDescr();
    void updateStatus();

    void clearGroup();

public slots:
    void updateTime();
    void updateDistance();

    void add(const QGeoCoordinate &p);

    //---------------------------------------
    // PROPERTIES
public:
    uint distance() const; //estimated total travel distance [m]
    void setDistance(uint v);

    uint time() const; //estimated total travel time [sec]
    void setTime(uint v);

    QAbstractListModel *mapModel() const;

protected:
    uint m_distance;
    uint m_time;
    QAbstractListModel *m_mapModel;

signals:
    void distanceChanged();
    void timeChanged();
};

template<class T, int miType>
class MissionGroupT : public MissionGroup
{
public:
    explicit MissionGroupT(VehicleMission *parent,
                           const QString &name,
                           const QString &title,
                           const QString &descr,
                           Fact *activeIndex = nullptr)
        : MissionGroup(parent, name, title, descr, activeIndex)
    {}
    int missionItemType() const { return miType; }

    MissionItem *createObject() { return new T(this); }
};
