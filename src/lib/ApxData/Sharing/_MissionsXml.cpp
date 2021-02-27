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
#include "MissionsXml.h"
#include <App/AppLog.h>
#include <Database/Database.h>
#define MISSIONS_XML_FORMAT 1

MissionsXmlExport::MissionsXmlExport(QString hash, QString fileName)
    : ShareXmlExport(Database::instance()->missions, "mission", MISSIONS_XML_FORMAT, title, fileName)
    , req(hash)
{}
bool MissionsXmlExport::run(QSqlQuery &query)
{
    if (!req.run(query))
        return false;
    info = req.info;
    details = req.details;
    return ShareXmlExport::run(query);
}
bool MissionsXmlExport::write(QDomNode &dom)
{
    writeInfo(dom, "details", details);
    //objects
    write(dom, "runways", "runway", req.mission.runways);
    write(dom, "waypoints", "waypoint", req.mission.waypoints);
    write(dom, "taxiways", "taxiway", req.mission.taxiways);
    write(dom, "points", "point", req.mission.pois);
    return true;
}
void MissionsXmlExport::write(QDomNode &dom,
                              const QString &sectionName,
                              const QString &elementName,
                              const QList<ProtocolMission::Item> &items)
{
    if (items.isEmpty())
        return;
    QDomDocument doc = dom.ownerDocument();
    QDomNode eg = dom.appendChild(doc.createElement(sectionName));
    eg.toElement().setAttribute("cnt", QString::number(items.size()));
    for (int i = 0; i < items.size(); ++i) {
        QDomNode e = eg.appendChild(doc.createElement(elementName));
        e.toElement().setAttribute("id", QString::number(i));
        const ProtocolMission::Item &item = items.at(i);
        if (!item.title.isEmpty())
            e.appendChild(doc.createElement("title")).appendChild(doc.createTextNode(item.title));
        e.appendChild(doc.createElement("lat"))
            .appendChild(doc.createTextNode(QVariant(item.lat).toString()));
        e.appendChild(doc.createElement("lon"))
            .appendChild(doc.createTextNode(QVariant(item.lon).toString()));
        foreach (QString key, item.details.keys()) {
            QString s = item.details.value(key).toString();
            if (s.isEmpty())
                continue;
            e.appendChild(doc.createElement(key)).appendChild(doc.createTextNode(s));
        }
    }
}

MissionsXmlImport::MissionsXmlImport(QString fileName)
    : ShareXmlImport(Database::instance()->missions, "mission", MISSIONS_XML_FORMAT, title, fileName)
{}

bool MissionsXmlImport::read(const QDomNode &dom)
{
    QVariantMap vmap = readInfo(dom, "details");
    details["topLeftLat"] = vmap.value("topLeftLat");
    details["topLeftLon"] = vmap.value("topLeftLon");
    details["bottomRightLat"] = vmap.value("bottomRightLat");
    details["bottomRightLon"] = vmap.value("bottomRightLon");
    details["distance"] = vmap.value("distance");
    details["callsign"] = vmap.value("callsign");
    details["runway"] = vmap.value("runway");
    int ecnt = 0;
    int gcnt;
    gcnt = read(dom, "runways", "runway", mission.runways);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;
    gcnt = read(dom, "waypoints", "waypoint", mission.waypoints);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;
    gcnt = read(dom, "taxiways", "taxiway", mission.taxiways);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;
    gcnt = read(dom, "points", "point", mission.pois);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;
    return ecnt > 0;
}
int MissionsXmlImport::read(const QDomNode &dom,
                            const QString &sectionName,
                            const QString &elementName,
                            QList<ProtocolMission::Item> &items)
{
    QDomElement e = dom.firstChildElement(sectionName);
    if (e.isNull())
        return 0;
    int ecnt = 0;
    int cnt = e.attribute("cnt").toInt();
    for (e = e.firstChildElement(elementName); !e.isNull(); e = e.nextSiblingElement(e.tagName())) {
        int id = e.attribute("id").toInt();
        if (id != items.size())
            return -1;
        QVariantMap values = readInfo(e, "");
        if (values.isEmpty())
            return -1;
        ProtocolMission::Item item;
        item.title = values.value("title").toString();
        item.lat = values.value("lat").toDouble();
        item.lon = values.value("lon").toDouble();
        values.remove("title");
        values.remove("lat");
        values.remove("lon");
        item.details = values;
        items.append(item);
        ecnt++;
    }
    if (ecnt != cnt)
        return -1;
    //qDebug()<<sectionName<<ecnt;
    return ecnt;
}
bool MissionsXmlImport::save(QSqlQuery &query)
{
    mission.title = info.value("title").toString();
    mission.lat = info.value("lat").toDouble();
    mission.lon = info.value("lon").toDouble();
    DBReqMissionsSave req(mission, details, info.value("time").toULongLong());
    if (!req.run(query))
        return false;
    info = req.info;
    return true;
}
