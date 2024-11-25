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

    f_path = new Fact(this, "open", tr("Path"), tr("Elevation files path"), Text, "import");
    f_path->setValue(path);
    // f_path->setDefaultValue(path);
    connect(f_path, &Fact::valueChanged, this, &ElevationMap::createElevationDatabase);

    createElevationDatabase();
    //  qml = loadQml("qrc:/ElevationPlugin.qml");
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
