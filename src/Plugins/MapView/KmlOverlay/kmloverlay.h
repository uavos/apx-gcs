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
#ifndef KMLOVERLAY_H
#define KMLOVERLAY_H

#include "kmlpolygonsmodel.h"
#include <Fact/Fact.h>
#include <QtCore>
#include <QtLocation>

class KmlOverlay : public Fact
{
    Q_OBJECT
    Q_PROPERTY(KmlPolygonsModel *kmlPolygons READ getKmlPolygons CONSTANT);
    Q_PROPERTY(QGeoCoordinate center READ getCenter NOTIFY centerChanged)
public:
    explicit KmlOverlay(Fact *parent = nullptr);

    Fact *f_open;
    Fact *f_visible;
    Fact *f_opacity;

    KmlPolygonsModel *getKmlPolygons() const;
    QGeoCoordinate getCenter() const;

    Q_INVOKABLE void updateKmlModels(const QGeoShape &shape);

private:
    KmlParser m_parser;
    QGeoCoordinate m_center;
    KmlPolygonsModel *m_kmlPolygons;
    QPointF gc2p(const QGeoCoordinate &c);
    QGeoCoordinate p2gc(const QPointF &p);

private slots:
    void onOpenTriggered();
    void onOverlayVisibleValueChanged();

signals:
    void centerChanged();
};

#endif //KMLOVERLAY_H
