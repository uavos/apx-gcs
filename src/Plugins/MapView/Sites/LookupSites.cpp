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
#include "LookupSites.h"
#include "Sites.h"
#include <Database/Database.h>
#include <Database/MissionsDB.h>

#include <App/AppRoot.h>
//=============================================================================
LookupSites::LookupSites(Sites *sites)
    : DatabaseLookup(sites,
                     "lookup",
                     tr("Nearest sites"),
                     tr("Database lookup"),
                     Database::instance()->missions)
    , sites(sites)
{
    setFilterEnabled(false);

    dbModel()->qmlMapSafe = true;
    dbModel()->ordered = false;

    connect(this, &LookupSites::areaChanged, this, &LookupSites::updateRect);

    disconnect(db, &DatabaseSession::modified, this, nullptr);
}
//=============================================================================
bool LookupSites::fixItemDataThr(QVariantMap *item)
{
    mutex.lock();
    QGeoCoordinate rc = reqCenter;
    double rd = reqDist;
    mutex.unlock();
    double dist = 0;
    QGeoCoordinate c(item->value("lat").toDouble(), item->value("lon").toDouble());
    if (rc.isValid()) {
        dist = rc.distanceTo(c);
        if (dist > rd)
            return false;
        item->insert("status", AppRoot::distanceToString(dist));
    }
    return true;
}
//=============================================================================
void LookupSites::updateRect()
{
    QGeoRectangle r = m_area.boundingGeoRectangle();
    mutex.lock();
    reqCenter = r.center();
    mutex.unlock();
    if ((!r.isValid()) || r.width() <= 0 || r.height() <= 0)
        return;
    double sz = qMax(1.0, qMax(r.width(), r.height()) * 2.0);
    r.setWidth(sz);
    r.setHeight(sz);
    while (!reqRect.isEmpty()) {
        double d = qMax(reqRect.width() / r.width(), reqRect.height() / r.height());
        if (d > 8.0)
            break;
        if (reqRect.contains(r)) {
            reloadQueryResults();
            return;
        }
        break;
    }
    r.setWidth(sz * 2.0);
    r.setHeight(sz * 2.0);
    reqRect = r;
    mutex.lock();
    reqDist = r.topLeft().distanceTo(r.bottomRight());
    mutex.unlock();
    defaultLookup();
}
//=============================================================================
void LookupSites::defaultLookup()
{
    if (reqRect.isEmpty())
        return;
    QGeoCoordinate c = reqRect.center();
    QVariantList v;
    v << c.latitude();
    v << c.latitude();
    v << std::pow(std::cos(qDegreesToRadians(c.latitude())), 2);
    v << c.longitude();
    v << c.longitude();
    query("SELECT *, ((?-lat)*(?-lat)*?)+((?-lon)*(?-lon)) AS dist FROM Sites"
          " ORDER BY dist ASC LIMIT 100",
          v);
    //qDebug()<<reqRect;
}
//=============================================================================
//=============================================================================
QGeoShape LookupSites::area() const
{
    return m_area;
}
void LookupSites::setArea(const QGeoShape &v)
{
    if (m_area == v)
        return;
    m_area = v;
    emit areaChanged();
}
//=============================================================================
