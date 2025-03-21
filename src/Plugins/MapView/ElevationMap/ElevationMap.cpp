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

#include "ElevationMap.h"
#include <App/App.h>
#include <App/AppSettings.h>
#include <Fleet/Fleet.h>
#include <Mission/MissionTools.h>
#include <Mission/UnitMission.h>
#include <Mission/Waypoint.h>
#include <Mission/Runway.h>
#include <Mission/Poi.h>

#include <QFileDialog>
#include <QMap>

ElevationMap::ElevationMap(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Elevation Map"),
           tr("Terrain elevation map"),
           Group | FlatModel,
           "elevation-rise")
{
    auto path = AppDirs::db().absolutePath() + "/Elevation";

    f_use = new Fact(this, "use", tr("Use elevation map"), "", Fact::Bool, "check");
    f_use->setValue(true);

    f_path = new Fact(this,
                      "open",
                      tr("Path"),
                      tr("Elevation files path"),
                      Text | PersistentValue,
                      "import");
    f_path->setDefaultValue(path);
    f_util = new Fact(this,
                         "util",
                         tr("Use util"),
                         tr("Use the special util"), 
                         Enum | PersistentValue,
                         "tools");
    f_util->setEnumStrings({"none", "gdallocationinfo"});

#ifdef Q_OS_LINUX
    f_util->setDefaultValue("none");
#else
    f_util->setDefaultValue("gdallocationinfo");
#endif

    connect(this, &Fact::pathChanged, this, &ElevationMap::getPluginEnableControl);
    connect(Fleet::instance(), &Fleet::currentChanged, this, [this]() { updateMission(); });
    connect(f_use, &Fact::valueChanged, this, &ElevationMap::changeExternalsVisibility);
    connect(f_path, &Fact::valueChanged, this, &ElevationMap::createElevationDatabase);
    connect(f_path, &Fact::triggered, this, &ElevationMap::onOpenTriggered);
    connect(f_util, &Fact::valueChanged, this, &ElevationMap::updateDBUtility);
    updateMission();
    createElevationDatabase();
    updateDBUtility();
    qml = loadQml("qrc:/ElevationPlugin.qml");
}

void ElevationMap::setCoordinateWithElevation(const QGeoCoordinate &coordinate)
{
    m_elevationDB->requestCoordinate(coordinate.latitude(), coordinate.longitude());
}

void ElevationMap::setElevationByCoordinate(const QGeoCoordinate &coordinate)
{
    m_elevationDB->requestElevation(coordinate.latitude(), coordinate.longitude());
}

void ElevationMap::setTerrainProfile(const QGeoPath &path)
{
    m_elevationDB->requestTerrainProfile(path);
}

void ElevationMap::createElevationDatabase()
{
    auto path = f_path->value().toString();
    m_elevationDB = QSharedPointer<OfflineElevationDB>::create(path);
    connect(m_elevationDB.data(), &OfflineElevationDB::coordinateReceived, this, &ElevationMap::setCoordinate);
    connect(m_elevationDB.data(), &OfflineElevationDB::elevationReceived, this, &ElevationMap::setElevation);
    connect(m_elevationDB.data(), &OfflineElevationDB::terrainProfileReceived, this, &ElevationMap::setGeoPath);
}

void ElevationMap::updateDBUtility()
{
    m_elevationDB->setUtil(static_cast<AbstractElevationDB::Util>(f_util->value().toInt()));
}

void ElevationMap::onOpenTriggered()
{
    QString path = QFileDialog::getExistingDirectory(nullptr,
                                                     tr("Open Directory"),
                                                     QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
    if (!path.isEmpty())
        f_path->setValue(path);
}

Unit *ElevationMap::unit() const
{
    return Fleet::instance()->current();
}

UnitMission *ElevationMap::mission() const
{
    return unit()->f_mission;
}

MissionTools *ElevationMap::missionTools() const 
{
    return mission()->f_tools;
}

Fact *ElevationMap::aglset() const
{
    return missionTools()->f_aglset;
}

void ElevationMap::updateMission()
{
    connect(mission(), &UnitMission::missionSizeChanged, this, &ElevationMap::changeExternalsVisibility);
    connect(missionTools()->f_aglsetApply, &Fact::triggered, this, &ElevationMap::setMissionAgl);
    changeExternalsVisibility();
}

void ElevationMap::getPluginEnableControl()
{
    f_control = AppSettings::instance()->findChild("application.plugins.elevationmap");
    if(f_control)
        connect(f_control, &Fact::valueChanged, this, &ElevationMap::changeExternalsVisibility);
}

void ElevationMap::setMissionAgl()
{
    auto m = mission();
    for (int i = 0; i < m->f_waypoints->size(); ++i) {
        auto wp = static_cast<Waypoint *>(m->f_waypoints->child(i));
        auto elevation = wp->elevation();
        if (qIsNaN(elevation))
            continue;

        int v = aglset()->value().toInt();
        v += static_cast<int>(elevation);
        wp->f_amsl->setValue(true);
        wp->f_altitude->setValue(v);

        // Add feets option
        v *= wp->M2FT_COEF;
        wp->f_altitude->setOpt("ft", v);
    }
}

void ElevationMap::changeExternalsVisibility()
{
    bool useValue{false};
    bool controlValue{false};
    if(f_control && !f_control->busy())
        controlValue = f_control->value().toBool();
    if (f_use)
        useValue = f_use->value().toBool();
    if (controlValue && useValue)
        setMissionValues(true);
    else
        setMissionValues(false);
}

void ElevationMap::setMissionValues(bool b)
{
    auto aglset = missionTools()->child("aglset");
    if (aglset)
        aglset->setVisible(b);

    // Signal missionSizeChanged is sent before mission is cleared
    auto m = mission();
    if(!m->missionSize() > 0) {
        clearMissionPoints(); 
        return;
    }

    setWaypointsValues(b);
    setRunwaysValues(b);
    setPoisValues(b);
}

void ElevationMap::setGeoPath(const QGeoPath &v)
{
    if (m_geoPath == v)
        return;
    m_geoPath = v;
    emit geoPathChanged(m_geoPath);
}

QGeoPath ElevationMap::geoPath() const
{
    return m_geoPath;
}

void ElevationMap::setCoordinate(const QGeoCoordinate &coordinate) {
    if(m_coordinate == coordinate)
        return;
    m_coordinate = coordinate;
    emit coordinateChanged(m_coordinate);
}

double ElevationMap::elevation() const
{
    return m_elevation;
}

void ElevationMap::setElevation(double v)
{
    if (m_elevation == v)
        return;

    m_elevation = v;
    emit elevationChanged();
}

void ElevationMap::setWaypointsValues(bool b)
{
    auto m = mission();
    QMap<QString, int> tempMap;
    for (int i = 0; i < m->f_waypoints->size(); ++i) {
        auto wp = static_cast<Waypoint *>(m->f_waypoints->child(i));
        wp->f_agl->setVisible(b);
        if (!b)
            continue;
        connect(this, &ElevationMap::coordinateChanged, wp, &Waypoint::extractElevation);
        connect(wp, &Waypoint::requestElevation, this, &ElevationMap::setCoordinateWithElevation);
        connect(wp, &Waypoint::requestTerrainProfile, this, &ElevationMap::setTerrainProfile);
        connect(this, &ElevationMap::geoPathChanged, wp, &Waypoint::buildTerrainProfile);
         // connect(wp, &Waypoint::coordinateChanged, this, &ElevationMap::setCoordinateWithElevation);  // for fast processing
        if(m->f_runways->size() > 0) {
            // For start point height update
            auto rw0 = static_cast<Runway *>(m->f_runways->child(0));
            auto rw0Hmsl = rw0->f_hmsl;
            connect(rw0, &Runway::elevationChanged, wp, &Waypoint::updateAgl);
            connect(rw0Hmsl, &Fact::valueChanged, wp, &Waypoint::updateAgl);
        }
        auto str = wp->coordinate().toString();
        auto alt = wp->f_altitude->value().toInt();
        if (!m_waypoints.contains(str) || m_waypoints[str] != alt) {
            setCoordinateWithElevation(wp->coordinate());
        }

        tempMap[str] = alt;
    }
    m_waypoints = tempMap;
}

void ElevationMap::setRunwaysValues(bool b) {
    auto m = mission();
    QSet<QString> tempSet;
    for (int i = 0; i < m->f_runways->size(); ++i) {
        if (!b)
            continue;
        auto runway = static_cast<Runway *>(m->f_runways->child(i));
        connect(this, &ElevationMap::coordinateChanged, runway, &Runway::extractElevation);
        connect(runway, &Runway::requestElevation, this, &ElevationMap::setCoordinateWithElevation);
        auto str = runway->coordinate().toString();
        if (!m_runways.contains(str)) {
            setCoordinateWithElevation(runway->coordinate());
        }
        tempSet.insert(str);
    }
    m_runways = tempSet;
}

void ElevationMap::setPoisValues(bool b) {
    
    auto m = mission();
    QSet<QString> tempSet;
    for (int i = 0; i < m->f_pois->size(); ++i) {
        if (!b)
            continue;
        auto poi = static_cast<Poi *>(m->f_pois->child(i));
        connect(this, &ElevationMap::coordinateChanged, poi, &Poi::extractElevation);
        connect(poi, &Poi::requestElevation, this, &ElevationMap::setCoordinateWithElevation);
        // connect(poi, &Poi::coordinateChanged, this, &ElevationMap::setCoordinateWithElevation);  // for fast processing
        auto str = poi->coordinate().toString();
        if (!m_pois.contains(str)) {
            setCoordinateWithElevation(poi->coordinate());
        }
        tempSet.insert(str);
    }
    m_pois = tempSet;
}

void ElevationMap::clearMissionPoints()
{
    m_waypoints.clear();
    m_runways.clear();
    m_pois.clear();
}
