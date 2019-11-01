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
#include "Updater.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppGcs.h>
//=============================================================================
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
    connect(f_check, &Fact::triggered, this, &Updater::check);

    //add menu to app
    Fact *m = new Fact(AppGcs::instance()->f_menu->app, f_check->name());
    m->setOpt("role", QAction::ApplicationSpecificRole);
    m->bind(f_check);

    initUpdaterImpl();

    connect(f_auto, &Fact::valueChanged, this, &Updater::updateAuto);
    updateAuto();

    App::jsync(this);
}

void Updater::initUpdaterImpl()
{
#ifdef Q_OS_MAC
    m_impl = std::make_unique<SparkleAutoUpdater>();
    m_impl->setFeedURL("https://uavos.github.io/apx-releases/appcast.xml");
#endif
#ifdef Q_OS_LINUX
    m_impl = new AppImageAutoUpdater(this);
#endif
}
//=============================================================================
void Updater::check()
{
    m_impl->checkForUpdates();
}
//=============================================================================
void Updater::checkInBackground()
{
    m_impl->checkForUpdatesInBackground();
}
//=============================================================================
void Updater::updateAuto()
{
    bool v = f_auto->value().toBool();
    m_impl->setAutomaticallyChecksForUpdates(v);
    if (v) {
        checkInBackground();
    }
}
//=============================================================================
