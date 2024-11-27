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

#include <Database/StorageSession.h>

namespace db {
namespace storage {

class SitesSave : public Request
{
    Q_OBJECT
public:
    explicit SitesSave(QString title, double lat, double lon, quint64 key = 0)
        : Request()
        , title(title)
        , lat(lat)
        , lon(lon)
        , key(key)
    {}

protected:
    bool run(QSqlQuery &query);

private:
    QString title;
    double lat;
    double lon;
    quint64 key;

signals:
    void siteAdded(QString title);
    void siteModified(QString title);
};

class SitesRemove : public Request
{
    Q_OBJECT
public:
    explicit SitesRemove(quint64 key)
        : Request()
        , key(key)
    {}

protected:
    bool run(QSqlQuery &query);

private:
    quint64 key;

signals:
    void siteRemoved();
};

class SitesFind : public Request
{
    Q_OBJECT
public:
    explicit SitesFind(double lat, double lon)
        : Request()
        , siteID(0)
        , lat(lat)
        , lon(lon)
    {}
    bool run(QSqlQuery &query);
    //result
    quint64 siteID;
    QString site;

private:
    double lat;
    double lon;
signals:
    void siteFound(quint64 siteID, QString site);
};

} // namespace storage
} // namespace db
