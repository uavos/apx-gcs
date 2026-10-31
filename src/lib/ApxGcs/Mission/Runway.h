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
#include <ApxMisc/DelayedEvent.h>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QtCore>

class Runway : public MissionItem
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate endPoint READ endPoint WRITE setEndPoint NOTIFY endPointChanged)
    Q_PROPERTY(QGeoCoordinate appPoint READ appPoint WRITE setAppPoint NOTIFY appPointChanged)
    Q_PROPERTY(double heading READ heading NOTIFY headingChanged)

    // landing pattern estimation (apx-fw ProcLD_runway)
    Q_PROPERTY(QGeoCoordinate taCenter READ taCenter NOTIFY taChanged)
    Q_PROPERTY(double taRadius READ taRadius NOTIFY taChanged)
    Q_PROPERTY(QGeoPath taPath READ taPath NOTIFY taChanged)

public:
    explicit Runway(MissionGroup *parent);

    Fact *f_type;
    Fact *f_approach;
    Fact *f_hmsl;
    Fact *f_dN;
    Fact *f_dE;

protected:
    QGeoRectangle boundingGeoRectangle() const;

private slots:
    void updateTitle();
    void updateMissionStartPoint();

    void scheduleTaUpdate();
    void updateTa();

private:
    struct PerfParams
    {
        double radius;    // shiva.perf.radius [m]
        double speed;     // shiva.perf.VC [m/s]
        double l1_period; // shiva.bearing.dist [s]
        double l1_damp;   // shiva.bearing.damp
        bool vtol;        // shiva.perf.type
    };
    PerfParams perfParams();
    Fact *shivaParam(const QString &path) const;
    MissionItem *lastWaypoint() const;
    QGeoPath estimateTaPath(const QGeoCoordinate &center,
                            double radius,
                            bool left,
                            const PerfParams &p) const;

    DelayedEvent _taUpdate{0, false}; // coalesce signals of one event loop pass only
    QPointer<MissionItem> m_lastWp;   // tracked last waypoint (signals connected)

    QGeoCoordinate m_taCenter;
    double m_taRadius{};
    QGeoPath m_taPath;

public slots:

    //---------------------------------------
    // PROPERTIES
public:
    QGeoCoordinate endPoint() const;
    void setEndPoint(const QGeoCoordinate &v);

    QGeoCoordinate appPoint() const;
    void setAppPoint(const QGeoCoordinate &v);

    double heading() const;

    QGeoCoordinate taCenter() const { return m_taCenter; }
    double taRadius() const { return m_taRadius; }
    QGeoPath taPath() const { return m_taPath; }

protected:
signals:
    void endPointChanged();
    void appPointChanged();
    void headingChanged();
    void taChanged();
};
