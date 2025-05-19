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
#include "UnitMission.h"

#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoRectangle>
#include <QtCore>
#include <QTimer>

#include <XbusMission.h>
#include <cmath>

// ==== Mission Analize ======
#include <QPointF>

class MissionItem : public Fact
{
    Q_OBJECT
    Q_PROPERTY(int missionItemType READ missionItemType CONSTANT)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

    Q_PROPERTY(QGeoPath geoPath READ geoPath NOTIFY geoPathChanged)
    Q_PROPERTY(double bearing READ bearing NOTIFY bearingChanged)
    Q_PROPERTY(double elevation READ elevation NOTIFY elevationChanged)
    Q_PROPERTY(uint time READ time NOTIFY timeChanged)
    Q_PROPERTY(uint distance READ distance NOTIFY distanceChanged)

    Q_PROPERTY(uint totalDistance READ totalDistance NOTIFY totalDistanceChanged)
    Q_PROPERTY(uint totalTime READ totalTime NOTIFY totalTimeChanged)

    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(bool isFeets READ isFeets WRITE setIsFeets NOTIFY isFeetsChanged)

    // ===== Mission analyze =======
    Q_PROPERTY(QList<QPointF> terrainProfile READ terrainProfile NOTIFY terrainProfileChanged)

public:
    explicit MissionItem(MissionGroup *parent,
                         const QString &name,
                         const QString &title,
                         const QString &descr);

    MissionGroup *group;
    int missionItemType() const;

    static constexpr float M2FT_COEF = 3.2808;  // conversion coefficient feets to meter
    static constexpr float M2KN_COEF = 1.9438;  // conversion coefficient meter per secont to knots
    static constexpr int TIMEOUT = 500;         // elevation update timeout
    Fact *f_order;
    Fact *f_latitude;
    Fact *f_longitude;

    Fact *f_elevationmap{nullptr};
    Fact *f_remove;
    Fact *f_feets;

    Q_INVOKABLE virtual QGeoRectangle boundingGeoRectangle() const;

    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

public slots:
    void extractElevation(const QGeoCoordinate &coordinate);
    void updatePath();
    void resetPath();

protected:
    virtual QGeoPath getPath();

    virtual void hashData(QCryptographicHash *h) const override;

    MissionItem *prevItem() const;
    MissionItem *nextItem() const;

    auto unit() const { return group->mission->unit; }
    void sendElevationRequest();
    void startTimer();

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

    double elevation() const; // terrain elevation in current wp
    void setElevation(double elevation);

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

    bool isFeets() const;
    void setIsFeets(bool v);

    void changeFeetMeters();

    // ===== Mission analyze ======
    QList<QPointF> terrainProfile() const;
    void setTerrainProfile(const QList<QPointF> &v);
    void clearTerrainProfile();

protected:
    QTimer m_timer;
    QTimer m_geoPathTimer;
    QGeoCoordinate m_coordinate;
    QList<QPointF> m_terrainProfile;
    QGeoPath m_terrainProfilePath;
    QGeoPath m_geoPath;
    double m_elevation{NAN};
    double m_bearing{};
    uint m_time{};
    uint m_distance{};

    uint m_totalDistance{};
    uint m_totalTime{};

    bool m_selected{};
    bool m_isFeets{};

signals:
    void coordinateChanged(QGeoCoordinate v);
    void geoPathChanged();
    void elevationChanged();
    void bearingChanged();
    void timeChanged();
    void distanceChanged();

    void totalDistanceChanged();
    void totalTimeChanged();

    void selectedChanged();
    void requestElevation(QGeoCoordinate v);
    void isFeetsChanged();

    // ===== Mission analyze ======
    void terrainProfileChanged();
};
