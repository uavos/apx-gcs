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

#include <Database/DatabaseLookup.h>
#include <QGeoCoordinate>
#include <QGeoRectangle>
#include <QtCore>
class Sites;

class LookupSites : public DatabaseLookup
{
    Q_OBJECT
    Q_PROPERTY(QGeoShape area READ area WRITE setArea NOTIFY areaChanged)

public:
    explicit LookupSites(Sites *sites);

private:
    Sites *sites;
    QGeoCoordinate reqCenter;
    double reqDist;
    QGeoRectangle reqRect;
    QMutex mutex;

protected:
    QVariantMap thr_prepareRecordData(const QJsonObject &jso) override;
    void defaultLookup() override;

private slots:
    void updateRect();

    //---------------------------------------
    // PROPERTIES
public:
    QGeoShape area() const;
    void setArea(const QGeoShape &v);

private:
    QGeoShape m_area;
signals:
    void areaChanged();
};
