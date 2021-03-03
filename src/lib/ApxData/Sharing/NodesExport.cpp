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
#include "NodesExport.h"
#include <App/App.h>
#include <Database/VehiclesDB.h>

#define NODES_FORMAT "11"

NodesExport::NodesExport(QObject *parent)
    : ShareExport("mission", "mission", parent)
{}

bool NodesExport::save(QString fileName, const ProtocolNode::Dict &d, QVariantMap info)
{
    if (!saveData(convert(d, info), fileName))
        return false;
    emit exported(fileName);
    return true;
}

QByteArray NodesExport::convert(const ProtocolNode::Dict &d, QVariantMap info)
{
    QJsonValue items; // = writeItems(d);
    if (items.isNull())
        return QByteArray();

    info.insert("format", NODES_FORMAT);
    info.insert("exported", QDateTime::currentDateTime().toString(Qt::RFC2822Date));
    info.insert("version", App::version());
    info.insert("type", _type);

    info.insert("items", items);

    QJsonObject json;
    json.insert(_name, QJsonObject::fromVariantMap(info));
    return QJsonDocument(json).toJson(QJsonDocument::Indented);
}
