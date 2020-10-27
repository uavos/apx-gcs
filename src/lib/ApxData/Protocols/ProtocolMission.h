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

#include "ProtocolBase.h"
#include "ProtocolNodeFile.h"

#include <QtCore>

#include <xbus/XbusMission.h>
#include <xbus/XbusNode.h>

class ProtocolVehicle;

class ProtocolMission : public ProtocolBase
{
    Q_OBJECT
    Q_ENUMS(ManeuverType)
    Q_ENUMS(RunwayType)

public:
    explicit ProtocolMission(ProtocolVehicle *vehicle);

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
    };

    enum ManeuverType {
        Direct,
        Path,
    };
    Q_ENUM(ManeuverType)

    enum RunwayType {
        Left = 0,
        Right,
    };
    Q_ENUM(RunwayType)

    static inline QString runwayTypeToString(int type)
    {
        return QMetaEnum::fromType<RunwayType>().valueToKey(type);
    }
    static inline RunwayType runwayTypeFromString(const QString &s)
    {
        return static_cast<RunwayType>(QMetaEnum::fromType<RunwayType>().keyToValue(s.toUtf8()));
    }
    static inline QString waypointTypeToString(int type)
    {
        return QMetaEnum::fromType<ManeuverType>().valueToKey(type);
    }
    static inline ManeuverType waypointTypeFromString(const QString &s)
    {
        return static_cast<ManeuverType>(QMetaEnum::fromType<ManeuverType>().keyToValue(s.toUtf8()));
    }

private:
    ProtocolVehicle *vehicle;

    QByteArray pack(const Mission &d);
    bool unpack(const QByteArray &data, Mission &d);

    static constexpr const char *nfile{"mission"};

    bool _enb{false};

private slots:
    void nodeNotify(ProtocolNode *protocol);
    void filesChanged();
    void fileUploaded();
    void fileDownloaded(const xbus::node::file::info_s &info, const QByteArray data);

public slots:
    void download();
    void upload(ProtocolMission::Mission d);

signals:
    void available();
    void uploaded();
    void downloaded(ProtocolMission::Mission d);
};

Q_DECLARE_METATYPE(ProtocolMission::Mission);
