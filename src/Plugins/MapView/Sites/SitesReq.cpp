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
#include "SitesReq.h"

using namespace db::storage;

bool SitesSave::run(QSqlQuery &query)
{
    if (std::isnan(lat) || std::isnan(lon) || lat == 0.0 || lon == 0.0)
        return false;
    if (!key) {
        //create site record
        query.prepare("INSERT INTO Sites"
                      " (title,lat,lon) VALUES(?,?,?)");
        query.addBindValue(title);
        query.addBindValue(lat);
        query.addBindValue(lon);
        if (!query.exec())
            return false;
        emit siteAdded(title);
    } else {
        //update site record
        query.prepare("UPDATE Sites SET"
                      " title=?,lat=?,lon=?"
                      " WHERE key=?");
        query.addBindValue(title);
        query.addBindValue(lat);
        query.addBindValue(lon);
        query.addBindValue(key);
        if (!query.exec())
            return false;
        emit siteModified(title);
    }
    return true;
}

bool SitesRemove::run(QSqlQuery &query)
{
    query.prepare("DELETE FROM Sites WHERE key=?");
    query.addBindValue(key);
    if (!query.exec())
        return false;
    emit siteRemoved();
    return true;
}

bool SitesFind::run(QSqlQuery &query)
{
    if (!(std::isnan(lat) || std::isnan(lon) || lat == 0.0 || lon == 0.0)) {
        query.prepare("SELECT (((?-lat)*(?-lat)*?)+((?-lon)*(?-lon))) AS dist, * FROM Sites"
                      " WHERE lat!=0 AND lon!=0 AND dist<2"
                      " ORDER BY dist ASC LIMIT 1");
        query.addBindValue(lat);
        query.addBindValue(lat);
        query.addBindValue(std::pow(std::cos(qDegreesToRadians(lat)), 2));
        query.addBindValue(lon);
        query.addBindValue(lon);
        if (!query.exec())
            return false;
        if (query.next()) {
            siteID = query.value("key").toULongLong();
            site = query.value("title").toString();
        }
    }
    emit siteFound(siteID, site);
    return true;
}
