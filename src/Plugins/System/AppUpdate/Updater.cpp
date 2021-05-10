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
#include "Updater.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppGcs.h>

Updater::Updater(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Check for updates"),
           tr("Update application"),
           Group)
{
    f_auto = new Fact(this,
                      "auto",
                      tr("Auto"),
                      tr("Automatically check for updates"),
                      Bool | PersistentValue);
    f_auto->setDefaultValue(true);

    f_check = new Fact(this, "update", tr("Check for updates"), title(), Action | Apply, "update");

    //add menu to app
    Fact *m = new Fact(AppGcs::instance()->f_menu->app, f_check->name());
    m->setOpt("role", QAction::ApplicationSpecificRole);
    m->setBinding(f_check);

    initUpdaterImpl();

    if (m_impl) {
        connect(f_check, &Fact::triggered, this, &Updater::check);
        connect(f_auto, &Fact::valueChanged, this, &Updater::updateAuto);
        updateAuto();
    } else {
        setEnabled(false);
    }

    App::jsync(this);
}

void Updater::initUpdaterImpl()
{
    m_impl = nullptr;
    if (!App::bundle())
        return;

#ifdef Q_OS_MAC
    m_impl = std::make_unique<SparkleAutoUpdater>();
    m_impl->setFeedURL("https://uavos.github.io/apx-gcs/docs/releases/appcast");
#endif

#ifdef Q_OS_LINUX
    m_impl = new AppImageAutoUpdater(this);
#endif
}

void Updater::check()
{
    if (m_impl)
        m_impl->checkForUpdates();
}

void Updater::checkInBackground()
{
    if (m_impl)
        m_impl->checkForUpdatesInBackground();
}

void Updater::updateAuto()
{
    bool v = f_auto->value().toBool();
    if (m_impl)
        m_impl->setAutomaticallyChecksForUpdates(v);
    if (v) {
        checkInBackground();
    }
}
