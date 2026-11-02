/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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
#include "WeatherModel.h"

WeatherModel::WeatherModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int WeatherModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _items.size();
}

QHash<int, QByteArray> WeatherModel::roleNames() const
{
    return {
        {CoordinateRole, "coordinate"},
        {LegRole, "leg"},
        {WaypointRole, "waypoint"},
        {DistRole, "dist"},
        {TrackRole, "track"},
        {AltMslRole, "altMsl"},
        {AltAglRole, "altAgl"},
        {WindSpeedRole, "windSpeed"},
        {WindDirRole, "windDir"},
        {TimeRole, "time"},
    };
}

QVariant WeatherModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= _items.size())
        return {};
    const auto &p = _items.at(index.row());
    switch (role) {
    case CoordinateRole:
        return QVariant::fromValue(p.coordinate);
    case LegRole:
        return p.leg;
    case WaypointRole:
        return p.waypoint;
    case DistRole:
        return p.dist;
    case TrackRole:
        return p.track;
    case AltMslRole:
        return p.altMsl;
    case AltAglRole:
        return p.altAgl;
    case WindSpeedRole:
        return p.windSpeed;
    case WindDirRole:
        return p.windDir;
    case TimeRole:
        return p.time;
    }
    return {};
}

void WeatherModel::reset(const QList<RoutePoint> &list)
{
    _all = list;
    rebuild();
}

void WeatherModel::setStride(int stride)
{
    stride = qMax(1, stride);
    if (_stride == stride)
        return;
    _stride = stride;
    rebuild();
}

void WeatherModel::rebuild()
{
    int k = qMax(_stride, int((_all.size() + shown_max - 1) / shown_max));
    QList<RoutePoint> shown;
    for (int i = 0; i < _all.size(); ++i) {
        const auto &p = _all.at(i);
        if (p.waypoint || i % k == 0)
            shown.append(p);
    }
    beginResetModel();
    _items = shown;
    endResetModel();
}
