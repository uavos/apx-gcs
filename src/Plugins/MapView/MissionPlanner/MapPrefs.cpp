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
#include "MapPrefs.h"

MapPrefs::MapPrefs(Fact *parent)
    : Fact(parent, "prefs", tr("Preferences"), tr("Map view settings"), Action | IconOnly, "wrench")
{
    f_provider = new Fact(this,
                          "provider",
                          tr("Tiles provider"),
                          tr("Name of map plugin"),
                          Text | PersistentValue,
                          "earth-box");
    f_provider->setDefaultValue("default");
    f_provider->setEnumStrings(QStringList() << "default"
                                             << "osm");

    f_type = new Fact(this,
                      "maptype",
                      tr("Map type"),
                      tr("Active map type"),
                      Text | PersistentValue,
                      "terrain");
}
