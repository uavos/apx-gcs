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
#include "AppQuickView.h"

#include <App/App.h>
#include <QCoreApplication>
#include <QScreen>
#include <QWidget>

AppQuickView::AppQuickView(const QString &name, const QString &title, QWindow *parent)
    : QQuickView(App::instance()->engine(), parent)
{
    setObjectName(name);
    setTitle(title);

    setFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    setColor(Qt::black);
    setVisibility(QWindow::AutomaticVisibility);

    setResizeMode(QQuickView::SizeRootObjectToView);

    const QRect &r = screen()->geometry();
    setWidth(r.width() / 2);
    setHeight(r.height() / 2);
    setMinimumSize(QSize(64, 64));

    _saveStateTimer.setSingleShot(true);
    _saveStateTimer.setInterval(100);
    connect(&_saveStateTimer, &QTimer::timeout, this, &AppQuickView::saveStateDo);

    connect(this, &QWindow::xChanged, this, &AppQuickView::saveState);
    connect(this, &QWindow::yChanged, this, &AppQuickView::saveState);
    connect(this, &QWindow::widthChanged, this, &AppQuickView::saveState);
    connect(this, &QWindow::heightChanged, this, &AppQuickView::saveState);
    connect(this, &QWindow::visibilityChanged, this, &AppQuickView::saveState);

    connect(App::instance(), &App::appQuit, this, &AppQuickView::hide);

    installEventFilter(this);

    restoreState();
}

void AppQuickView::saveState()
{
    _saveStateTimer.start();
}
void AppQuickView::saveStateDo()
{
    if (visibility() == QWindow::Minimized)
        return;
    if (visibility() == QWindow::Hidden)
        return;

    bool fullscreen = visibility() == QWindow::FullScreen || visibility() == QWindow::Maximized
                      || width() == screen()->availableSize().width()
                      || height() == screen()->availableSize().height();

    QSettings s;
    s.beginGroup("windows");
    if (!fullscreen)
        s.setValue(objectName() + "_geometry", geometry());
    s.setValue(objectName() + "_maximized", fullscreen);
}

void AppQuickView::restoreState()
{
    QSettings s;
    s.beginGroup("windows");
    setGeometry(s.value(objectName() + "_geometry", geometry()).toRect());
    if (s.value(objectName() + "_maximized").toBool())
        showFullScreen();
    else
        showNormal();

    raise();
}

bool AppQuickView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        QCloseEvent e;
        e.setAccepted(event->isAccepted());
        emit closed(&e);
        event->setAccepted(e.isAccepted());
    }
    return QObject::eventFilter(obj, event);
}
