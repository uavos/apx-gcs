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
#ifndef WidgetQmlItem_H
#define WidgetQmlItem_H

#include <QtQuick>

class QWidget;

class WidgetQmlItem : public QQuickPaintedItem
{
    Q_OBJECT

public:
    WidgetQmlItem(QQuickItem *parent = nullptr);
    virtual ~WidgetQmlItem() override;

    void paint(QPainter *painter) override;

    Q_INVOKABLE void setWidget(QWidget *w);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    /*//route mouse events
    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
    virtual void hoverMoveEvent(QHoverEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void routeHoverEvents(QHoverEvent *event);
    void routeMouseEvents(QMouseEvent *event);
    void routeWheelEvents(QWheelEvent *event);*/

    virtual bool event(QEvent *e) override;

private:
    QWidget *m_widget;

private slots:
    void updateWidgetSize();
};

#endif
