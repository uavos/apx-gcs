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
#ifndef LookupMissions_H
#define LookupMissions_H
//=============================================================================
#include <Database/DatabaseLookup.h>
#include <Fact/Fact.h>
#include <QGeoCoordinate>
#include <QtCore>
class VehicleMission;
//=============================================================================
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
//=============================================================================
#endif
