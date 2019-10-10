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
#include "AppMenu.h"

#include <App/App.h>
#include <App/AppRoot.h>

#include <Nodes/Nodes.h>
#include <Nodes/NodesShare.h>

#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicles.h>

#include <QDesktopServices>
//=============================================================================
AppMenu::AppMenu(Fact *parent)
    : Fact(parent, "sysmenu", tr("Menu"), tr("Application system menu"), Group, "menu")
{
    Fact *f;

    app = new Fact(this, "app", tr("Application"), "", Group, "application");
    f = new Fact(app,
                 "about",
                 QString("%1 %2").arg(tr("About")).arg(QCoreApplication::applicationName()),
                 "",
                 NoFlags,
                 "information");
    connect(f, &Fact::triggered, App::instance(), &App::about);

    f = new Fact(app, "preferences", tr("Preferences"), "", NoFlags, "settings");
    connect(f, &Fact::triggered, AppRoot::instance(), &Fact::trigger);

    file = new Fact(this, "file", tr("File"), "", Group, "file");
    f = new Fact(file, "telemetry");
    f->setOpt("shortcut", QKeySequence::Open);
    f->bind(Vehicles::instance()->f_replay->f_telemetry->f_share->f_import);
    f = new Fact(file, "nodes");
    f->bind(Vehicles::instance()->f_replay->f_nodes->f_share->f_import);

    vehicle = new Fact(this, "vehicle", tr("Vehicle"), "", Group, "airplane");
    vehicle->bind(Vehicles::instance()->f_select);

    tools = new Fact(this, "tools", "", "", Group);
    tools->bind(AppRoot::instance()->f_tools);

    windows = new Fact(this, "windows", "", "", Group);
    windows->bind(AppRoot::instance()->f_windows);

    help = new Fact(this, "help", tr("Help"), "", Group, "help");
    f = new Fact(help, "mandala", tr("Mandala Report"), "", NoFlags, "format-list-bulleted");
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("http://127.0.0.1:9080/mandala?descr"));
    });
    f = new Fact(help, "docs", tr("Documentation"), "", NoFlags, "help-circle");
    f->setOpt("shortcut", QKeySequence::HelpContents);
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("http://docs.uavos.com"));
    });
    f = new Fact(help, "changelog", tr("Changelog"), "", NoFlags, "delta");
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("http://uavos.github.io/apx-releases/CHANGELOG.html"));
    });
    f = new Fact(help, "issue", tr("Report a problem"), "", NoFlags, "bug");
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/uavos/apx-releases/issues"));
    });
    createMenuBar();
}
//=============================================================================
QMenuBar *AppMenu::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(nullptr);
    for (int i = 0; i < size(); ++i) {
        Fact *g = child(i);
        Fact *gm = g->menu();
        QMenu *menu = menuBar->addMenu(g->title());
        for (int j = 0; j < gm->size(); ++j) {
            Fact *f = gm->child(j);
            if (f->menu())
                f = f->menu();
            menu->addAction(f->title());
        }
    }
    return menuBar;
}
//=============================================================================
