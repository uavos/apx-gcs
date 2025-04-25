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
#include <QFutureWatcher>

class Waypoint : public MissionItem
{
    Q_OBJECT

    Q_PROPERTY(bool reachable READ reachable WRITE setReachable NOTIFY reachableChanged)
    Q_PROPERTY(bool warning READ warning WRITE setWarning NOTIFY warningChanged)
    Q_PROPERTY(bool collision READ collision WRITE setCollision NOTIFY collisionChanged)
    Q_PROPERTY(ChosenFact chosen READ chosen WRITE setChosen NOTIFY chosenChanged)
    Q_PROPERTY(int unsafeAgl READ unsafeAgl CONSTANT)

public:
    struct TerrainInfo
    {
        QGeoPath terrainProfilePath;
        QList<QPointF> terrainProfile;
        double minHeight;
        double maxHeight;
    };

    enum ChosenFact {
        ALT = 0,
        AGL,
    };
    Q_ENUM(ChosenFact)

    explicit Waypoint(MissionGroup *parent);

    Fact *f_altitude;
    Fact *f_amsl;
    Fact *f_agl;

    Fact *f_atrack;
    Fact *f_xtrack;

    WaypointActions *f_actions;

    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

protected:
    QGeoPath getPath() override;
    void calcAltitude();
    void recalcAltitude();
    void processAgl();
    void calcAgl();

    // Feets processing
    void calcAltitudeFt();
    void processAglFt();
    void calcAglFt();

private:
    QString _altUnits;
    double m_terrainProfileMin{0};
    double m_terrainProfileMax{200};

private slots:
    double getStartHMSL();
    void updateTitle() override;
    void updateDescr();
    void updateAMSL();
    void updateAltDescr();
    void sendTerrainProfileRequest();
    static void createTerrainInfo(QPromise<TerrainInfo> &promise, const QGeoPath &path);
    void updateMinMaxHeight();
    void updateTerrainInfo();
    void setAglEnabled();

public slots:
    void initElevationMap();
    void updateAgl();
    void buildTerrainProfile(const QGeoPath &path);
    void checkCollision();

    //---------------------------------------
    // PROPERTIES
public:
    ChosenFact chosen() const;
    void setChosen(ChosenFact v);

    double minHeight() const;
    void setMinHeight(const double v);

    double maxHeight() const;
    void setMaxHeight(const double v);

    double worstRouteAgl() const;
    void setWorstRouteAgl(const double v);

    bool reachable() const;
    void setReachable(bool v);

    bool warning() const;
    void setWarning(bool v);

    bool collision() const;
    void setCollision(bool v);

    int unsafeAgl() const;

protected:
    static const int UNSAFE_AGL = 100; // Suggested by the CEO
    QGeoCoordinate m_worstRouteAglCoordinate;
    QFutureWatcher<TerrainInfo> m_watcher;
    ChosenFact m_chosen{ALT};
    double m_minHeight{0};
    double m_maxHeight{200};
    double m_worstRouteAgl{100}; // TODO specify maxUnsafeAgl in Ctor
    bool m_reachable{};
    bool m_warning{};
    bool m_collision{};

    // ==== Debug purposes only =====
    // void testInsert();
    // QTimer m_testTimer;
    // ==============================

signals:
    void requestTerrainProfile(QGeoPath v);
    void minHeightChanged();
    void maxHeightChanged();
    void reachableChanged();
    void collisionChanged();
    void warningChanged();
    void chosenChanged();
};
