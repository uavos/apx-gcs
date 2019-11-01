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
#ifndef AppWindow_H
#define AppWindow_H
#include "AppPlugins.h"
#include <Fact/Fact.h>
#include <QWindow>
//=============================================================================
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
//=============================================================================
#endif
