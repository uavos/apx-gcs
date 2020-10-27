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
#include "AppSettings.h"
#include <App/App.h>
#include <App/AppDirs.h>
//=============================================================================
AppSettings *AppSettings::_instance = nullptr;
AppSettings::AppSettings(Fact *parent)
    : Fact(parent,
           "settings",
           tr("Preferences"),
           tr("Application settings"),
           Group | FlatModel,
           "settings")
{
    _instance = this;

    f_interface = new Fact(this, "interface", tr("Interface"), "", Section);
    f_graphics = new Fact(this, "graphics", tr("Graphics"), "", Section);
    f_application = new Fact(this, "application", tr("Application"), "", Section);
    Fact *item;

    item = new Fact(f_interface,
                    "lang",
                    tr("Language"),
                    tr("Interface localization"),
                    Enum | PersistentValue);
    QStringList st;
    st << "default";
    st.append(App::instance()->languages());
    item->setEnumStrings(st);

    item = new Fact(f_graphics, "scale", tr("Scale"), tr("UI scale factor"), Float | PersistentValue);
    item->setDefaultValue(1.0);
    //item->setPrecision(1);
    item->setMin(0.5);
    item->setMax(2.0);
    scaleEvent.setInterval(1000);
    connect(item, &Fact::valueChanged, &scaleEvent, &DelayedEvent::schedule);
    connect(&scaleEvent, &DelayedEvent::triggered, this, [item]() {
        App::instance()->setScale(item->value().toDouble());
    });
    App::instance()->setScale(item->value().toDouble());

    item = new Fact(f_graphics,
                    "opengl",
                    tr("Accelerate graphics"),
                    tr("Enable OpenGL graphics when supported"),
                    Enum | PersistentValue);
    st.clear();
    st << "default";
    st << "OpenGL";
    st << "OpenGL ES 2.0";
    st << "OpenVG";
    item->setEnumStrings(st);

    item = new Fact(f_graphics,
                    "smooth",
                    tr("Smooth animations"),
                    tr("Enable animations and antialiasing"),
                    Bool | PersistentValue);
    item->setDefaultValue(true);

    item = new Fact(f_graphics,
                    "antialiasing",
                    tr("Antialiasing"),
                    tr("Enable antialiasing"),
                    Enum | PersistentValue);
    st.clear();
    st << "off";
    st << "minimum";
    st << "maximum";
    item->setDefaultValue(st.at(2));
    item->setEnumStrings(st);

    item = new Fact(f_graphics,
                    "effects",
                    tr("Effects"),
                    tr("Graphical effects level"),
                    Enum | PersistentValue);
    item->setEnumStrings(st);
    item->setDefaultValue(st.at(2));

    Fact *f = new Fact(f_graphics, "test", tr("Highlight instruments"), "", Bool);
    f->setValue(false);

    App::jsync(this);

    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("smooth"));
    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("antialiasing"));
    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("effects"));
    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("test"));
}
//=============================================================================
