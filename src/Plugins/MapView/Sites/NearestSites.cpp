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
#include "NearestSites.h"
#include "Sites.h"

#include <Database/StorageSession.h>

NearestSites::NearestSites(Sites *sites)
    : Fact(sites, "db", tr("Nearest sites"), tr("Sites Database"), Group, "database-search")
    , _sites(sites)
{
    _dbmodel = new DatabaseModel(this);
    setModel(_dbmodel);
    setOpt("page", "Menu/FactMenuPageLookupDB.qml");

    connect(_dbmodel, &DatabaseModel::requestRecordsList, this, &NearestSites::dbRequestRecordsList);
    connect(_dbmodel, &DatabaseModel::requestRecordInfo, this, &NearestSites::dbRequestRecordInfo);
    // connect(_dbmodel, &DatabaseModel::itemTriggered, this, &NearestSites::loadNodeConfig);

    connect(this, &NearestSites::areaChanged, this, &NearestSites::updateRect);
}

void NearestSites::dbRequestRecordsList()
{
    if (_reqRect.isEmpty())
        return;

    QString s = "SELECT key, ((?-lat)*(?-lat)*?)+((?-lon)*(?-lon))"
                " AS dist FROM Sites"
                " ORDER BY dist ASC LIMIT 100";

    auto c = _reqRect.center();
    auto v = QVariantList({
        c.latitude(),
        c.latitude(),
        std::pow(std::cos(qDegreesToRadians(c.latitude())), 2),
        c.longitude(),
        c.longitude(),
    });

    auto req = new db::storage::Request(s, v);
    connect(req,
            &db::storage::Request::queryResults,
            _dbmodel,
            &DatabaseModel::dbUpdateRecords,
            Qt::QueuedConnection);
    req->exec();
}

void NearestSites::dbRequestRecordInfo(quint64 key)
{
    auto req = new db::storage::Request("SELECT * FROM Sites WHERE key=?", {key});
    connect(
        req,
        &db::storage::Request::queryResults,
        this,
        [this, key](QJsonArray records) {
            auto jso = records.first().toObject();

            _mutex.lock();
            auto rc = _reqCenter;
            auto rd = _reqDist;
            _mutex.unlock();

            if (!rc.isValid())
                return;

            double dist = 0;
            QGeoCoordinate c(jso.value("lat").toVariant().toDouble(),
                             jso.value("lon").toVariant().toDouble());

            if (!c.isValid())
                return;

            jso["value"] = AppRoot::distanceToString(rc.distanceTo(c));
            // qDebug() << key << jso;
            _dbmodel->setRecordModelInfo(key, jso);
        },
        Qt::QueuedConnection);
    req->exec();
}

void NearestSites::updateRect()
{
    QGeoRectangle r = m_area.boundingGeoRectangle();

    _mutex.lock();
    _reqCenter = r.center();
    _mutex.unlock();

    if ((!r.isValid()) || r.width() <= 0 || r.height() <= 0)
        return;
    double sz = qMax(1.0, qMax(r.width(), r.height()) * 2.0);
    r.setWidth(sz);
    r.setHeight(sz);
    while (!_reqRect.isEmpty()) {
        double d = qMax(_reqRect.width() / r.width(), _reqRect.height() / r.height());
        if (d > 8.0)
            break;
        if (_reqRect.contains(r)) {
            // defaultLookup();
            return;
        }
        break;
    }
    r.setWidth(sz * 2.0);
    r.setHeight(sz * 2.0);
    _reqRect = r;

    _mutex.lock();
    _reqDist = r.topLeft().distanceTo(r.bottomRight());
    _mutex.unlock();

    dbRequestRecordsList();
}

QGeoShape NearestSites::area() const
{
    return m_area;
}
void NearestSites::setArea(const QGeoShape &v)
{
    if (m_area == v)
        return;
    m_area = v;
    emit areaChanged();
}
