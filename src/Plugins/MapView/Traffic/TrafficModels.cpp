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
#include "TrafficModels.h"

AircraftModel::AircraftModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int AircraftModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _items.size();
}

QHash<int, QByteArray> AircraftModel::roleNames() const
{
    return {
        {HexRole, "hex"},
        {CallsignRole, "callsign"},
        {TypeRole, "type"},
        {CategoryRole, "category"},
        {CategoryTextRole, "categoryText"},
        {IconRole, "icon"},
        {IconScaleRole, "iconScale"},
        {CoordinateRole, "coordinate"},
        {OnGroundRole, "onGround"},
        {AltRole, "altM"},
        {AltSrcRole, "altSrc"},
        {AltBaroRole, "altBaroM"},
        {AltGeomRole, "altGeomM"},
        {GsRole, "gsKt"},
        {TrackRole, "track"},
        {VsRole, "vsMs"},
        {DistRole, "distKm"},
        {BrgRole, "brg"},
        {LevelRole, "level"},
    };
}

// ADS-B emitter category (DO-260B) -> description, map icon and its relative size
struct CategoryInfo
{
    const char *text;
    const char *icon;
    double scale;
};
static CategoryInfo categoryInfo(const QString &c)
{
    static const QHash<QString, CategoryInfo> map{
        {"A1", {"Light aircraft", "airplane", 0.8}},
        {"A2", {"Small aircraft", "airplane", 0.9}},
        {"A3", {"Large aircraft", "airplane", 1.0}},
        {"A4", {"High vortex aircraft", "airplane", 1.1}},
        {"A5", {"Heavy aircraft", "airplane", 1.2}},
        {"A6", {"High performance aircraft", "airplane", 1.0}},
        {"A7", {"Rotorcraft", "helicopter", 0.9}},
        {"B1", {"Glider", "airplane", 0.8}},
        {"B2", {"Lighter-than-air", "airballoon", 0.9}},
        {"B3", {"Parachutist", "parachute", 0.8}},
        {"B4", {"Ultralight", "airplane", 0.7}},
        {"B6", {"UAV", "quadcopter", 0.8}},
        {"B7", {"Space vehicle", "rocket", 0.9}},
        {"C1", {"Emergency vehicle", "car", 0.7}},
        {"C2", {"Service vehicle", "car", 0.7}},
        {"C3", {"Obstacle", "radio-tower", 0.8}},
        {"C4", {"Obstacle", "radio-tower", 0.8}},
        {"C5", {"Obstacle", "radio-tower", 0.8}},
        {"C6", {"Obstacle", "radio-tower", 0.8}},
        {"C7", {"Obstacle", "radio-tower", 0.8}},
    };
    return map.value(c, {"Aircraft", "airplane", 1.0});
}

QVariant AircraftModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= _items.size())
        return {};
    const auto &a = _items.at(index.row());
    switch (role) {
    case HexRole:
        return a.hex;
    case CallsignRole:
        return a.callsign.isEmpty() ? a.hex.toUpper() : a.callsign;
    case TypeRole:
        return a.type;
    case CategoryRole:
        return a.category;
    case CategoryTextRole:
        return QString(categoryInfo(a.category).text);
    case IconRole:
        return QString(categoryInfo(a.category).icon);
    case IconScaleRole:
        return categoryInfo(a.category).scale;
    case CoordinateRole:
        return QVariant::fromValue(a.coordinate);
    case OnGroundRole:
        return a.onGround;
    case AltRole:
        return a.altM();
    case AltSrcRole:
        return a.altSrc();
    case AltBaroRole:
        return a.altBaroM;
    case AltGeomRole:
        return a.altGeomM;
    case GsRole:
        return a.gsKt;
    case TrackRole:
        return a.track;
    case VsRole:
        return a.vsMs;
    case DistRole:
        return a.distKm;
    case BrgRole:
        return a.brg;
    case LevelRole:
        return a.level;
    }
    return {};
}

void AircraftModel::update(const QList<Aircraft> &list)
{
    // drop aircraft missing in the reply
    for (int i = _items.size() - 1; i >= 0; --i) {
        bool found = false;
        for (const auto &a : list) {
            if (a.hex == _items.at(i).hex) {
                found = true;
                break;
            }
        }
        if (found)
            continue;
        beginRemoveRows({}, i, i);
        _items.removeAt(i);
        endRemoveRows();
    }
    // update or append
    for (const auto &incoming : list) {
        int idx = -1;
        for (int i = 0; i < _items.size(); ++i) {
            if (_items.at(i).hex == incoming.hex) {
                idx = i;
                break;
            }
        }
        if (idx >= 0) {
            _items[idx] = incoming;
            emit dataChanged(index(idx), index(idx));
            continue;
        }
        beginInsertRows({}, _items.size(), _items.size());
        _items.append(incoming);
        endInsertRows();
    }
}

void AircraftModel::clear()
{
    if (_items.isEmpty())
        return;
    beginResetModel();
    _items.clear();
    endResetModel();
}

bool AircraftModel::contains(const QString &hex) const
{
    for (const auto &a : _items) {
        if (a.hex == hex)
            return true;
    }
    return false;
}

// airspaces

AirspaceModel::AirspaceModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int AirspaceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _items.size();
}

QHash<int, QByteArray> AirspaceModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {LabelRole, "label"},
        {ColorRole, "color"},
        {CenterRole, "center"},
        {PathRole, "path"},
    };
}

QVariant AirspaceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= _items.size())
        return {};
    const auto &a = _items.at(index.row());
    switch (role) {
    case NameRole:
        return a.name;
    case LabelRole:
        return a.label;
    case ColorRole:
        return a.color;
    case CenterRole:
        return QVariant::fromValue(a.center);
    case PathRole:
        return a.path;
    }
    return {};
}

void AirspaceModel::reset(const QList<Airspace> &list)
{
    beginResetModel();
    _items = list;
    endResetModel();
}
