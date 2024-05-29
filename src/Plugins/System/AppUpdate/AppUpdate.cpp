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
#include "AppUpdate.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppGcs.h>

#include "AppUpdateChecker.h"

AppUpdate::AppUpdate(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("App updates"),
           tr("Check for software updates"),
           Group | ProgressTrack)
{
    f_auto = new Fact(this,
                      "auto",
                      tr("Auto"),
                      tr("Automatically check for updates"),
                      Bool | PersistentValue);
    f_auto->setDefaultValue(true);

    f_checker = new AppUpdateChecker(this);

    f_check = new Fact(this, "update", tr("Check for updates"), title(), Action | Apply, "update");
    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop checking"), Action | Stop, "stop");
    f_stop->setEnabled(false);

    connect(f_checker, &Fact::progressChanged, f_check, [this]() {
        f_check->setEnabled(f_checker->progress() < 0);
        f_stop->setEnabled(!f_check->enabled());
    });

    //add menu to app
    Fact *m = new Fact(AppGcs::instance()->f_menu->app, f_check->name());
    m->setOpt("role", QAction::ApplicationSpecificRole);
    m->setBinding(f_check);

    connect(f_check, &Fact::triggered, this, &Fact::trigger);
    connect(f_check, &Fact::triggered, f_checker, &AppUpdateChecker::checkForUpdates);
    bindProperty(f_checker, "progress", true);

    connect(f_stop, &Fact::triggered, f_checker, &AppUpdateChecker::abort);

    App::jsync(this);
}
