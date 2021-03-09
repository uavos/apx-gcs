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
#include "MissionImport.h"
#include <App/App.h>
#include <Database/MissionsDB.h>

#define MISSION_FORMAT 1

MissionImport::MissionImport(QObject *parent)
    : ShareImport("mission", "mission", parent)
{}

bool MissionImport::load(QString fileName)
{
    QByteArray data = loadData(fileName);
    if (data.isEmpty())
        return false;

    ProtocolMission::Mission d;
    QVariantMap info;

    if (!convert(data, &d, &info))
        return false;

    QString title = d.title;

    DBReqMissionsSave *req = new DBReqMissionsSave(d, info);
    connect(
        req,
        &DBReqMissionsSave::missionHash,
        this,
        [this, fileName, title](QString hash) { emit imported(fileName, hash, title); },
        Qt::QueuedConnection);
    req->exec();
    return true;
}

bool MissionImport::convert(const QByteArray &data, ProtocolMission::Mission *d, QVariantMap *info)
{
    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        apxMsgW() << err.errorString();
        return false;
    }
    do {
        if (!json.isObject())
            break;
        if (!json.object().contains(_name))
            break;
        QJsonObject m = json[_name].toObject();
        if (!m.contains("items"))
            break;
        if (!m.contains("lat"))
            break;
        if (!m.contains("lon"))
            break;

        d->title = m["title"].toString();
        d->lat = m["lat"].toDouble();
        d->lon = m["lon"].toDouble();
        qDebug() << d->title << d->lat << d->lon;
        QJsonObject items = m["items"].toObject();
        read(items, "rw", &d->runways);
        read(items, "wp", &d->waypoints);
        read(items, "poi", &d->pois);
        read(items, "tw", &d->taxiways);
        QVariantMap v = m.toVariantMap();
        v.remove("title");
        v.remove("lat");
        v.remove("lon");
        v.remove("items");
        *info = v;
        //qDebug() << v;
        return true;
    } while (0);
    apxMsgW() << tr("file format error");
    return false;
}

void MissionImport::read(QJsonObject &json, const QString &name, QList<ProtocolMission::Item> *items)
{
    if (!json.contains(name))
        return;

    for (auto i : json[name].toArray()) {
        ProtocolMission::Item item;
        QVariantMap v = i.toObject().toVariantMap();
        item.title = v["title"].toString();
        item.lat = v["lat"].toDouble();
        item.lon = v["lon"].toDouble();
        v.remove("title");
        v.remove("lat");
        v.remove("lon");
        item.details = v;
        items->append(item);
    }
    qDebug() << name + ":" << items->size();
}
