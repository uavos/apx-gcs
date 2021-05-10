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

#include <QtWidgets>

class DrawingArea : public QWidget
{
    Q_OBJECT
public:
    DrawingArea();

public:
    void resizeArea(int width, int height);
    void addData(double x, double y);
    void startTrace();
    void stopTrace();

    void clearTrace();

    QString sAxis[2];
    double bX, bY, sX, sY;

private:
    QPolygonF getPolygon(int k = 25);
    void DrawData();
    double dScale, dOffset;
    bool isTraced;
    QPointF cVector;
    QList<QPointF> cVectorList;
    double xMin, xMax, yMin, yMax;

private:
    QPixmap *pixMap;
    QPainter *painter;
    QPointF DPoint(double x, double y);
    QPointF DPoint(QPointF pt);

protected:
    void paintEvent(QPaintEvent *p);
    void resizeEvent(QResizeEvent *event);
    int heightForWidth(int w) const { return w; }
};
