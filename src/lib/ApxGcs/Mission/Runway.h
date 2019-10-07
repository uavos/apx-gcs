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
#ifndef Runway_H
#define Runway_H
//=============================================================================
#include "MissionItem.h"
#include <QGeoCoordinate>
#include <QtCore>
//=============================================================================
class Runway : public MissionItem
{
    Q_OBJECT
    Q_ENUMS(RunwayType)
    Q_PROPERTY(QGeoCoordinate endPoint READ endPoint WRITE setEndPoint NOTIFY endPointChanged)
    Q_PROPERTY(QGeoCoordinate appPoint READ appPoint WRITE setAppPoint NOTIFY appPointChanged)
    Q_PROPERTY(double heading READ heading NOTIFY headingChanged)

public:
    explicit Runway(MissionGroup *parent);

    Fact *f_type;
    Fact *f_approach;
    Fact *f_hmsl;
    Fact *f_dN;
    Fact *f_dE;

    enum RunwayType {
        Left = 0,
        Right,
    };
    Q_ENUM(RunwayType)

protected:
    QGeoRectangle boundingGeoRectangle() const;

private slots:
    void updateTitle();
    void updateMissionStartPoint();

public slots:

    //---------------------------------------
    // PROPERTIES
public:
    QGeoCoordinate endPoint() const;
    void setEndPoint(const QGeoCoordinate &v);

    QGeoCoordinate appPoint() const;
    void setAppPoint(const QGeoCoordinate &v);

    double heading() const;

protected:
signals:
    void endPointChanged();
    void appPointChanged();
    void headingChanged();
};
//=============================================================================
#endif
