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
#include "MapPrefs.h"
#include <App/App.h>
#include <Mission/VehicleMission.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
MapPrefs::MapPrefs(Fact *parent)
    : Fact(parent, "prefs", tr("Preferences"), tr("Map view settings"), Action | IconOnly, "wrench")
{
    QSettings *settings = AppSettings::settings();
    f_provider = new AppSettingFact(settings,
                                    this,
                                    "provider",
                                    tr("Tiles provider"),
                                    tr("Name of map plugin"),
                                    Text,
                                    "uavos");
    f_provider->setIcon("earth-box");
    f_provider->setEnumStrings(QStringList() << "uavos"
                                             << "osm");
    f_provider->load();

    f_type = new AppSettingFact(settings,
                                this,
                                "maptype",
                                tr("Map type"),
                                tr("Active map type"),
                                Text);
    f_type->setIcon("terrain");
    f_type->load();
}
//=============================================================================
