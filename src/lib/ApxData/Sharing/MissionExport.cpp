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
#include "MissionExport.h"
#include <App/App.h>
#include <Database/MissionsDB.h>

#define MISSION_FORMAT "11"

MissionExport::MissionExport(QObject *parent)
    : ShareExport("mission", "mission", parent)
{}

bool MissionExport::save(QString fileName, const ProtocolMission::Mission &d, QVariantMap info)
{
    if (!saveData(convert(d, info), fileName))
        return false;
    emit exported(fileName);
    return true;
}

QByteArray MissionExport::convert(const ProtocolMission::Mission &d, QVariantMap info)
{
    QJsonValue items = writeItems(d);
    if (items.isNull())
        return QByteArray();

    info.insert("format", MISSION_FORMAT);

    info.insert("title", d.title);
    info.insert("lat", d.lat);
    info.insert("lon", d.lon);

    info.insert("exported", QDateTime::currentDateTime().toString(Qt::RFC2822Date));
    info.insert("version", App::version());
    info.insert("type", _type);

    info.insert("items", items);

    QJsonObject json;
    json.insert(_name, QJsonObject::fromVariantMap(info));
    return QJsonDocument(json).toJson(QJsonDocument::Indented);
}

QJsonValue MissionExport::writeItems(const ProtocolMission::Mission &d)
{
    QJsonObject json;
    write(json, "rw", d.runways);
    write(json, "wp", d.waypoints);
    write(json, "poi", d.pois);
    write(json, "tw", d.taxiways);
    return json;
}

void MissionExport::write(QJsonObject &json,
                          const QString &name,
                          const QList<ProtocolMission::Item> &items)
{
    if (items.isEmpty())
        return;

    QJsonArray array;
    for (auto const &i : items) {
        QJsonObject obj;
        if (!i.title.isEmpty())
            obj.insert("title", i.title);
        obj.insert("lat", i.lat);
        obj.insert("lon", i.lon);
        for (auto k : i.details.keys()) {
            QString s = i.details.value(k).toString();
            if (s.isEmpty())
                continue;
            obj.insert(k, s);
        }
        array.append(obj);
    }

    json.insert(name, array);
}
