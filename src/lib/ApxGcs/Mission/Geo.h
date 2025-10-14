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

#include "MissionItem.h"

#include <QGeoPolygon>
#include <QtCore>

class Geo : public MissionItem
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *pointsModel READ pointsModel CONSTANT)
    Q_PROPERTY(
        QGeoCoordinate radiusPoint READ radiusPoint WRITE setRadiusPoint NOTIFY radiusPointChanged)

    Q_PROPERTY(QGeoPolygon polygon READ polygon NOTIFY polygonChanged)
    Q_PROPERTY(QGeoCoordinate polyC1 READ polyC1 NOTIFY polyHandleChanged)
    Q_PROPERTY(QGeoCoordinate polyC2 READ polyC2 NOTIFY polyHandleChanged)

public:
    explicit Geo(MissionGroup *parent);

    Fact *f_role;
    Fact *f_shape;
    Fact *f_top;
    Fact *f_bottom;

    Fact *f_label;

    // geometry
    Fact *f_radius;
    Fact *f_points;
    MissionPoint *f_p2;

    Q_INVOKABLE void addPoint(QGeoCoordinate c, int n = -1);
    Q_INVOKABLE void removePoint(int n);

    QGeoRectangle boundingGeoRectangle() const override;

    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

private:
    QGeoCoordinate getPolyHandle(bool prev) const;

private slots:
    void updateTitle() override;
    void updatePolygon();
    void updateActive();

    //---------------------------------------
    // PROPERTIES
public:
    QAbstractListModel *pointsModel() const { return m_pointsModel; }

    QGeoCoordinate radiusPoint() const;
    void setRadiusPoint(const QGeoCoordinate &v);

    auto polygon() const { return m_polygon; }
    // points to add new vertex
    QGeoCoordinate polyC1() const { return getPolyHandle(true); }
    QGeoCoordinate polyC2() const { return getPolyHandle(false); }

protected:
    QAbstractListModel *m_pointsModel;
    QGeoPolygon m_polygon;

signals:
    void radiusPointChanged();
    void polygonChanged();
    void polyHandleChanged();
};
