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
#ifndef PluginInterface_H
#define PluginInterface_H
#include <QtPlugin>
//=============================================================================
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
//=============================================================================
QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(PluginInterface, "com.uavos.gcs.PluginInterface/1.0")
QT_END_NAMESPACE
//=============================================================================
#endif
