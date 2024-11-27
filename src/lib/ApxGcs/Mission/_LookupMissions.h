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

#include <Database/DatabaseLookup.h>
#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QtCore>
class VehicleMission;

class LookupMissions : public DatabaseLookup
{
    Q_OBJECT

public:
    explicit LookupMissions(VehicleMission *mission,
                            Fact *parent,
                            FactBase::Flags flags = FactBase::Flags(Group));

private:
    VehicleMission *mission;
    QGeoCoordinate reqPoint;

protected:
    bool fixItemDataThr(QVariantMap *item) override;
    void defaultLookup() override;

private slots:
    void loadItem(QVariantMap modelData);

public slots:
    void dbLookupMissionsByArea(QGeoCoordinate c, QString siteName = QString());
};
