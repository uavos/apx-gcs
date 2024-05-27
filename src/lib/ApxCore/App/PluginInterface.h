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

#include <QtPlugin>

class PluginInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~PluginInterface() {}

    enum PluginFlags {
        //Plugin type
        Feature = 0x0000, //called init() and createControl(), Fact added to Tools
        Widget = 0x0001,  //window created on demand
        PluginTypeMask = 0x0007,
        //Options
        Restore = 1 << 4,  //restore window on startup
        Launcher = 2 << 4, //show quick launcher icon on main screen
        //Tools Sections
        PluginSectionMask = 0xF << 16,
        System = 1 << 16,
        Tool = 2 << 16,
        Map = 3 << 16,
    };

    // called after loading when enabled in preferences
    virtual void init() {}

    // called if FeaturePlugin on demand to create Fact or QWidget
    virtual QObject *createControl() { return nullptr; }

    virtual int flags() { return Feature; }
    virtual QString title() { return QString(); }
    virtual QString descr() { return QString(); }
    virtual QString icon() { return QString(); }

    virtual QStringList depends() { return QStringList(); }

    virtual bool closeEvent() { return true; }
};

Q_DECLARE_INTERFACE(PluginInterface, "com.uavos.gcs.PluginInterface/1.0")
