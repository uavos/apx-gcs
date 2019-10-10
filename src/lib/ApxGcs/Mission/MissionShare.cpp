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
#include "MissionShare.h"
#include "MissionStorage.h"
#include "VehicleMission.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Sharing/MissionsXml.h>
#include <Vehicles/Vehicle.h>
//=============================================================================
MissionShare::MissionShare(VehicleMission *mission, Fact *parent, Flags flags)
    : Share(parent, tr("Mission"), "mission", AppDirs::missions(), QStringList(), flags)
    , mission(mission)
{
    connect(this, &Share::imported, mission->storage, &MissionStorage::loadMission);
}
//=============================================================================
QString MissionShare::defaultExportFileName() const
{
    QString fname = mission->site().replace(" ", "");
    if (!fname.isEmpty())
        fname.append("-");
    fname.append(mission->vehicle->callsign());
    QString subj = mission->f_title->text().simplified();
    if (!subj.isEmpty())
        fname.append(QString("-%1").arg(subj));
    else
        fname.append(QString("-%1").arg(mission->missionSize()));
    fname.replace(' ', '-');
    return fname;
}
ShareXmlExport *MissionShare::exportRequest(QString title, QString fileName)
{
    QString hash = mission->storage->dbHash;
    if (hash.isEmpty()) {
        apxMsgW() << tr("Missing config in database");
        return nullptr;
    }
    return new MissionsXmlExport(hash, title, fileName);
}
ShareXmlImport *MissionShare::importRequest(QString title, QString fileName)
{
    return new MissionsXmlImport(title, fileName);
}
//=============================================================================
//=============================================================================
