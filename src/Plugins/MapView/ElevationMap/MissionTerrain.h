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

#include <QGeoCoordinate>
#include <QObject>
#include <QTimer>

#include <cmath>

class Fact;
class MissionItem;
class UnitMission;

// Terrain data the plugin keeps for a mission and its items.
// The mission library knows nothing about terrain: these objects are created by
// the ElevationMap plugin as children of the mission objects they describe (so
// they live exactly as long as those objects) and are removed when the plugin
// turns passive.

// Mission wide values: takeoff elevation, height range for the chart, collision flag
class MissionTerrain : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double startElevation READ startElevation NOTIFY startElevationChanged)
    Q_PROPERTY(double minHeight READ minHeight NOTIFY minHeightChanged)
    Q_PROPERTY(double maxHeight READ maxHeight NOTIFY maxHeightChanged)
    Q_PROPERTY(bool collision READ collision NOTIFY collisionChanged)

public:
    explicit MissionTerrain(UnitMission *mission);

    // the object of the mission, created on the first call
    static MissionTerrain *of(UnitMission *mission);

    double startElevation() const { return m_startElevation; }
    void setStartElevation(double v);

    double minHeight() const { return m_minHeight; }
    void setMinHeight(double v);

    double maxHeight() const { return m_maxHeight; }
    void setMaxHeight(double v);

    bool collision() const { return m_collision; }
    void setCollision(bool v);

public slots:
    void checkCollision();
    void updateMinHeight();
    void updateMaxHeight();
    void setDefaultMinMaxHeight();

private:
    UnitMission *m_mission;
    double m_startElevation{0};
    double m_minHeight{0};
    double m_maxHeight{200};
    bool m_collision{false};

signals:
    void startElevationChanged();
    void minHeightChanged();
    void maxHeightChanged();
    void collisionChanged();
};

// Terrain elevation under a mission item (runway, point of interest, waypoint)
class ItemTerrain : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double elevation READ elevation NOTIFY elevationChanged)

public:
    explicit ItemTerrain(MissionItem *item);

    // the object of the item, nullptr when the plugin has not attached one
    static ItemTerrain *of(MissionItem *item);

    static constexpr int TIMEOUT = 500; // elevation update timeout, ms

    MissionItem *item() const { return m_item; }

    double elevation() const { return m_elevation; }
    void setElevation(double v);

    // shows the elevation next to the editor of this field of the item
    void setInfoField(Fact *f);

    // removes what the plugin added to the item; called before the object is deleted
    virtual void detach();

public slots:
    // the plugin broadcasts every elevation it gets: take the one for this item
    void extractElevation(const QGeoCoordinate &coordinate);
    void sendElevationRequest();

protected:
    MissionItem *m_item;
    double m_elevation{NAN};

private:
    QTimer m_timer;
    Fact *m_infoField{nullptr};

signals:
    void elevationChanged();
    void requestElevation(QGeoCoordinate v);
};
