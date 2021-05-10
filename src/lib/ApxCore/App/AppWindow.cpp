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
#include "AppWindow.h"
#include <App/App.h>
#include <QCoreApplication>
#include <QScreen>
#include <QWidget>

AppWindow::AppWindow(Fact *parent, AppPlugin *plugin)
    : Fact(parent,
           plugin->name.toLower(),
           plugin->interface->title(),
           plugin->interface->descr(),
           Bool,
           plugin->interface->icon())
    , plugin(plugin)
    , w(nullptr)
    , blockWidgetVisibilityEvent(true)
{
    saveStateTimer.setSingleShot(true);
    saveStateTimer.setInterval(100);
    connect(&saveStateTimer, &QTimer::timeout, this, &AppWindow::saveStateDo);

    if (plugin->interface->flags() & PluginInterface::Restore) {
        loadPresistentValue();
        updateWidget();
    }
    connect(this, &Fact::valueChanged, this, &AppWindow::updateWidget);
    connect(this, &Fact::triggered, this, [this]() {
        if (value().toBool())
            updateWidget();
        else
            setValue(true);
    });

    if (plugin->interface->flags() & PluginInterface::Restore) {
        connect(this, &Fact::valueChanged, this, &Fact::savePresistentValue, Qt::QueuedConnection);
    }
}

void AppWindow::updateWidget()
{
    if (value().toBool()) {
        if (!plugin->control) {
            QObject *obj = plugin->interface->createControl();
            if (!obj)
                return;
            plugin->control = obj;
        }
        if (!w) {
            w = qobject_cast<QWidget *>(plugin->control);
            if (!w)
                return;
            connect(w, &QWidget::destroyed, this, [this]() {
                w = nullptr;
                plugin->control = nullptr;
                setValue(false);
            });
            connect(App::instance(),
                    &App::applicationStateChanged,
                    this,
                    &AppWindow::applicationStateChanged);
            connect(App::instance(),
                    &App::visibilityChanged,
                    this,
                    &AppWindow::applicationVisibilityChanged);

            w->setWindowIcon(QApplication::windowIcon());

            if (!plugin->interface->title().isEmpty())
                w->setWindowTitle(plugin->interface->title());
            //qDebug()<<w->windowFlags();
            /*w->setWindowFlags(
            Qt::Window
            |Qt::WindowTitleHint
            |Qt::WindowSystemMenuHint
            |Qt::WindowCloseButtonHint
            |Qt::WindowFullscreenButtonHint
            );
      w->setAttribute(Qt::WA_MacAlwaysShowToolWindow);*/
        }
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
        restoreState();
        w->show();
        w->raise();

        connect(App::instance(), &App::appQuit, w, [this]() {
            blockWidgetVisibilityEvent = true;
            w->close();
        });

        QWindow *wh = w->windowHandle();
        if (wh) {
            connect(wh,
                    &QWindow::visibilityChanged,
                    this,
                    &AppWindow::widgetVisibilityChanged,
                    Qt::UniqueConnection);
            connect(wh, &QWindow::xChanged, this, &AppWindow::saveState);
            connect(wh, &QWindow::yChanged, this, &AppWindow::saveState);
            connect(wh, &QWindow::widthChanged, this, &AppWindow::saveState);
            connect(wh, &QWindow::heightChanged, this, &AppWindow::saveState);

            if (wh->visibility() == QWindow::Minimized)
                w->showNormal();

            //if(wh->visibility()==QWindow::FullScreen)w->showNormal();
            //wh->showNormal();
            //wh->raise();
            //QTimer::singleShot(100,wh,&QWindow::showNormal);
            //connect(qApp, &QCoreApplication::aboutToQuit, wh, &QWindow::hide);
        }
        return;
    }
    if (!w)
        return;
    if (!w->close()) {
        setValue(true);
        return;
    }
}

void AppWindow::applicationStateChanged(Qt::ApplicationState state)
{
    //qDebug() << state << w;
    if (!w)
        return;
    QWindow *app_w = App::instance()->window();
    if (!app_w)
        return;

    if (state == Qt::ApplicationActive) {
        if (!(w->isFullScreen() || app_w->visibility() == QWindow::FullScreen)) {
            w->raise();
        }
    }
}
void AppWindow::applicationVisibilityChanged(QWindow::Visibility visibility)
{
    if (!w)
        return;
    //qDebug() << visibility;

    QWindow *window = w->windowHandle();
    if (!window)
        return;
    blockWidgetVisibilityEvent = true;
    if (visibility == QWindow::Minimized) {
        if (w->isFullScreen())
            w->showNormal();
        w->hide();
    } else if (visibility != QWindow::Hidden) {
        w->show();
    }
    blockWidgetVisibilityEvent = false;
}

void AppWindow::widgetVisibilityChanged(QWindow::Visibility visibility)
{
    //qDebug() << visibility;
    if (blockWidgetVisibilityEvent)
        return;
    QWindow *window = qobject_cast<QWindow *>(sender());
    if (!window)
        return;
    blockWidgetVisibilityEvent = true;
    if (visibility == QWindow::Hidden)
        setValue(false);
    /*else if(visibility==QWindow::Minimized){
    //w->show();
  }*/
    //saveState();
    blockWidgetVisibilityEvent = false;
}

void AppWindow::saveState()
{
    saveStateTimer.start();
}
void AppWindow::saveStateDo()
{
    if (!w)
        return;
    QWindow *window = w->windowHandle();
    if (!window)
        return;
    if (w->isHidden())
        return;
    if (w->isMinimized())
        return;
    if (w->isMaximized())
        return;
    if (window->visibility() == QWindow::Minimized)
        return;
    if (window->visibility() == QWindow::Maximized)
        return;
    if (window->visibility() == QWindow::Hidden)
        return;

    bool bFullScreen = w->isFullScreen() || window->visibility() == QWindow::FullScreen;
    if (bFullScreen)
        return;
    if (!bFullScreen) {
        if (window->width() == window->screen()->availableSize().width()
            || window->height() == window->screen()->availableSize().height())
            return;
    }
    QSettings s;
    s.beginGroup(parentFact()->name());
    s.setValue(name(), w->saveGeometry());
}

void AppWindow::restoreState()
{
    if (!w)
        return;
    QSettings s;
    s.beginGroup(parentFact()->name());
    w->restoreGeometry(s.value(name()).toByteArray());
}

bool AppWindow::showLauncher()
{
    return plugin->interface->flags() & PluginInterface::Launcher;
}
