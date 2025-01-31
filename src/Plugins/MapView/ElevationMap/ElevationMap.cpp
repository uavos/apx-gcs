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

#include <QFileDialog>

ElevationMap::ElevationMap(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Elevation Map"),
           tr("Terrain elevation map"),
           Group | FlatModel,
           "elevation-rise")
{
    m_elevation = qQNaN();
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

    connect(this, &Fact::pathChanged, this, &ElevationMap::getPluginEnableControl);
    connect(Fleet::instance(), &Fleet::currentChanged, this, [this]() { updateMission(); });
    connect(f_use, &Fact::valueChanged, this, &ElevationMap::changeExternalsVisibility);
    connect(f_path, &Fact::valueChanged, this, &ElevationMap::createElevationDatabase);
    connect(f_path, &Fact::triggered, this, &ElevationMap::onOpenTriggered);

    updateMission();
    createElevationDatabase();
    qml = loadQml("qrc:/ElevationPlugin.qml");
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

void ElevationMap::setElevationByCoordinate(const QGeoCoordinate &v)
{
    auto elevation = m_elevationDB->getElevation(v.latitude(), v.longitude());
    setElevation(elevation);
}

double ElevationMap::getElevationByCoordinate(const QGeoCoordinate &v)
{
    return m_elevationDB->getElevation(v.latitude(), v.longitude());
}

void ElevationMap::createElevationDatabase()
{
    auto path = f_path->value().toString();
    m_elevationDB = std::make_shared<OfflineElevationDB>(path);
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
        auto elevation = getElevationByCoordinate(wp->coordinate());
        if (qIsNaN(elevation))
            continue;

        int v = aglset()->value().toInt();
        v += static_cast<int>(elevation);
        wp->f_amsl->setValue(true);
        wp->f_altitude->setValue(v);
    }
}

void ElevationMap::changeExternalsVisibility()
{
    if(!f_control || !f_use) {
        setMissionValues(false);
        return;
    }
    bool controlValue;
    if(!f_control->busy())
        controlValue = f_control->value().toBool();
    auto useValue = f_use->value().toBool();
    if (controlValue && useValue) {
        setMissionValues(true);
    } else {
        setMissionValues(false);
    }
}

void ElevationMap::setMissionValues(bool b)
{
    auto m = mission();
    for (int i = 0; i < m->f_waypoints->size(); ++i) {
        auto wp = static_cast<Waypoint *>(m->f_waypoints->child(i));
        wp->f_agl->setVisible(b);
    }
    auto aglset = missionTools()->child("aglset");
    if (aglset)
        aglset->setVisible(b);
}
