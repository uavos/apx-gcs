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
        {CoordinateRole, "coordinate"},
        {AltRole, "altFt"},
        {TrackRole, "track"},
        {DistRole, "distKm"},
        {BrgRole, "brg"},
        {LevelRole, "level"},
        {LabelRole, "label"},
        {NumberRole, "number"},
    };
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
    case CoordinateRole:
        return QVariant::fromValue(a.coordinate);
    case AltRole:
        return a.altFt;
    case TrackRole:
        return a.track;
    case DistRole:
        return a.distKm;
    case BrgRole:
        return a.brg;
    case LevelRole:
        return a.level;
    case NumberRole:
        return a.number;
    case LabelRole: {
        // "CSN3083  11550 ft  288 kt"
        QString s = a.callsign.isEmpty() ? a.hex.toUpper() : a.callsign;
        s += a.altFt > 0 ? QString("  %1 ft").arg(qRound(a.altFt)) : QString("  GND");
        if (a.vsFpm > 200)
            s += QString::fromUtf8(" ↑");
        else if (a.vsFpm < -200)
            s += QString::fromUtf8(" ↓");
        if (a.gsKt > 0)
            s += QString("  %1 kt").arg(qRound(a.gsKt));
        return s;
    }
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
        Aircraft a = incoming;
        if (idx >= 0) {
            a.number = _items.at(idx).number;
            _items[idx] = a;
            emit dataChanged(index(idx), index(idx));
            continue;
        }
        a.number = _nextNumber++;
        beginInsertRows({}, _items.size(), _items.size());
        _items.append(a);
        endInsertRows();
    }
}

void AircraftModel::clear()
{
    if (_items.isEmpty()) {
        _nextNumber = 1;
        return;
    }
    beginResetModel();
    _items.clear();
    endResetModel();
    _nextNumber = 1;
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
