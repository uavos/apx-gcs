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
#ifndef DictMission_H
#define DictMission_H
#include <QtCore>
//=============================================================================
class DictMission : private QObject
{
    Q_OBJECT
public:
    DictMission(QObject *parent = nullptr);

    enum RunwayType { Left = 0, Right };
    Q_ENUM(RunwayType)
    static inline QString runwayTypeToString(int type)
    {
        return QMetaEnum::fromType<RunwayType>().valueToKey(type);
    }
    static inline RunwayType runwayTypeFromString(const QString &s)
    {
        return static_cast<RunwayType>(QMetaEnum::fromType<RunwayType>().keyToValue(s.toUtf8()));
    }

    enum WaypointType { Hdg = 0, Line };
    Q_ENUM(WaypointType)
    static inline QString waypointTypeToString(int type)
    {
        return QMetaEnum::fromType<WaypointType>().valueToKey(type);
    }
    static inline WaypointType waypointTypeFromString(const QString &s)
    {
        return static_cast<WaypointType>(QMetaEnum::fromType<WaypointType>().keyToValue(s.toUtf8()));
    }

    struct Item
    {
        QString title;
        qreal lat;
        qreal lon;
        QVariantMap details;
    };

    struct Mission
    {
        QString title;
        qreal lat;
        qreal lon;

        QList<Item> runways;
        QList<Item> waypoints;
        QList<Item> taxiways;
        QList<Item> pois;

        Mission();
        void reset();
    };

    //current state
    Mission mission;
};
//=============================================================================
Q_DECLARE_METATYPE(DictMission::Mission)
//=============================================================================
#endif
