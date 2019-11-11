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
#include "MissionsXml.h"
#include <App/AppLog.h>
#include <Database/Database.h>
#define MISSIONS_XML_FORMAT 1
//=============================================================================
MissionsXmlExport::MissionsXmlExport(QString hash, QString title, QString fileName)
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
                              const QList<DictMission::Item> &items)
{
    if (items.isEmpty())
        return;
    QDomDocument doc = dom.ownerDocument();
    QDomNode eg = dom.appendChild(doc.createElement(sectionName));
    eg.toElement().setAttribute("cnt", QString::number(items.size()));
    for (int i = 0; i < items.size(); ++i) {
        QDomNode e = eg.appendChild(doc.createElement(elementName));
        e.toElement().setAttribute("id", QString::number(i));
        const DictMission::Item &item = items.at(i);
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
//=============================================================================
//=============================================================================
MissionsXmlImport::MissionsXmlImport(QString title, QString fileName)
    : ShareXmlImport(Database::instance()->missions, "mission", MISSIONS_XML_FORMAT, title, fileName)
{}
//=============================================================================
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
                            QList<DictMission::Item> &items)
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
        DictMission::Item item;
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
//=============================================================================
//=============================================================================
bool MissionsXmlImport::readOldFormat(const QDomNode &dom, int fmt)
{
    if (fmt > 0) {
        apxMsgW() << tr("Format not supported") << fmt;
        return false;
    }
    QDomElement e = dom.toElement();
    if (e.tagName() != tagName && e.tagName() != "flight_plan")
        return false;

    QString r_title = e.attribute("title");
    r_title.remove("from telemetry", Qt::CaseInsensitive);
    r_title.remove("mission", Qt::CaseInsensitive);
    r_title = r_title.simplified();
    if (!r_title.isEmpty()) {
        title = r_title;
    }
    title.remove(info.value("callsign").toString(), Qt::CaseInsensitive);
    title.replace('-', ' ');
    title.replace('_', ' ');
    title = title.simplified();
    info["title"] = title;

    QDateTime dateTime = QDateTime::fromString(e.firstChildElement("timestamp").text());
    qint64 time = dateTime.isValid() ? dateTime.toMSecsSinceEpoch() : 0;
    if (time <= 0) {
        time = defaultTime;
        qWarning() << tr("No timestamp in file") << QString("(%1)").arg(tagName);
    }
    info["time"] = time;

    int ecnt = 0;
    int gcnt;
    gcnt = readOldFormat(dom, "runways", "runway", mission.runways);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;
    gcnt = readOldFormat(dom, "waypoints", "waypoint", mission.waypoints);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;
    gcnt = readOldFormat(dom, "taxiways", "taxiway", mission.taxiways);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;
    gcnt = readOldFormat(dom, "points", "point", mission.pois);
    if (gcnt < 0)
        return false;
    ecnt += gcnt;

    //set mission pos
    if (mission.runways.size() > 0) {
        info["lat"] = mission.runways.first().lat;
        info["lon"] = mission.runways.first().lon;
    } else if (mission.taxiways.size() > 0) {
        info["lat"] = mission.taxiways.first().lat;
        info["lon"] = mission.taxiways.first().lon;
    } else if (mission.waypoints.size() > 0) {
        info["lat"] = mission.waypoints.first().lat;
        info["lon"] = mission.waypoints.first().lon;
    }

    return ecnt > 0;
}
int MissionsXmlImport::readOldFormat(const QDomNode &dom,
                                     const QString &sectionName,
                                     const QString &elementName,
                                     QList<DictMission::Item> &items)
{
    QDomElement e = dom.firstChildElement(sectionName);
    int cnt = -1;
    if (!e.isNull()) {
        cnt = e.attribute("cnt").toInt();
    } else {
        e = dom.toElement();
    }
    int ecnt = 0;
    for (e = e.firstChildElement(elementName); !e.isNull(); e = e.nextSiblingElement(e.tagName())) {
        int id = e.attribute("idx").toInt();
        if (cnt > 0 && id != items.size())
            return -1;
        QVariantMap values = readInfo(e, "");
        if (values.isEmpty())
            return -1;
        DictMission::Item item;
        item.lat = values.value(values.contains("latitude") ? "latitude" : "lat").toDouble();
        item.lon = values.value(values.contains("longitude") ? "longitude" : "lon").toDouble();
        if (values.contains("HMSL"))
            item.details["hmsl"] = values.value("HMSL");
        if (values.contains("turnR"))
            item.details["radius"] = values.value("turnR");
        if (values.contains("dN"))
            item.details["dN"] = values.value("dN");
        if (values.contains("dE"))
            item.details["dE"] = values.value("dE");
        if (values.contains("altitude"))
            item.details["altitude"] = values.value("altitude");
        if (values.contains("loops"))
            item.details["loops"] = values.value("loops");
        if (values.contains("time"))
            item.details["timeout"] = values.value("time");

        if (e.tagName() == "waypoint") {
            if (values.contains("type"))
                item.details["type"] = values.value("type").toUInt() > 0 ? "Line" : "Hdg";
            if (values.contains("agl"))
                item.details["altitude"] = values.value("agl");
            QVariantMap a = readInfo(e, "actions");
            QStringList st;
            foreach (QString key, a.keys()) {
                QString v = a.value(key).toString();
                if (v.isEmpty())
                    continue;
                if (v.toDouble() == 0.0)
                    continue;
                key = key.toLower();
                if (key == "turnR")
                    key = "radius";
                st.append(QString("%1=%2").arg(key).arg(v));
            }
            if (!st.isEmpty())
                item.details["actions"] = st.join(',');
        } else if (e.tagName() == "runway") {
            if (values.contains("turn"))
                item.details["type"] = values.value("turn").toUInt() > 0 ? "Right" : "Left";
            if (values.contains("approach"))
                item.details["approach"] = values.value("approach");
        }
        foreach (QString key, item.details.keys()) {
            if (item.details.value(key).toString().isEmpty())
                item.details.remove(key);
        }

        items.append(item);
        ecnt++;
    }
    if (cnt >= 0 && ecnt != cnt)
        return -1;
    //qDebug()<<sectionName<<ecnt;
    return ecnt;
}
//=============================================================================
