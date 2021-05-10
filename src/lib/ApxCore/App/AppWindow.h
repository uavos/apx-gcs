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

#include "AppPlugins.h"
#include <Fact/Fact.h>
#include <QWindow>

class AppWindow : public Fact
{
    Q_OBJECT
    Q_PROPERTY(bool showLauncher READ showLauncher CONSTANT)

public:
    explicit AppWindow(Fact *parent, AppPlugin *plugin);

private:
    AppPlugin *plugin;
    QWidget *w;
    bool blockWidgetVisibilityEvent;

    QTimer saveStateTimer;

    bool showLauncher();

private slots:
    void updateWidget();
    void applicationStateChanged(Qt::ApplicationState state);
    void applicationVisibilityChanged(QWindow::Visibility visibility);
    void widgetVisibilityChanged(QWindow::Visibility visibility);

    void saveState();
    void saveStateDo();
    void restoreState();
};
