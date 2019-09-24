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
#ifndef LookupSites_H
#define LookupSites_H
//=============================================================================
#include <Database/DatabaseLookup.h>
#include <QGeoCoordinate>
#include <QGeoRectangle>
#include <QtCore>
class Sites;
//=============================================================================
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
    bool fixItemDataThr(QVariantMap *item);

private slots:
    void updateRect();
    void dbLookup();

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
//=============================================================================
#endif
