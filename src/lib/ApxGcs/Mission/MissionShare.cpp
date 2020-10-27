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
#include "MissionShare.h"
#include "MissionStorage.h"
#include "VehicleMission.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Sharing/MissionsXml.h>
#include <Vehicles/Vehicle.h>

MissionShare::MissionShare(VehicleMission *mission, Fact *parent, Flags flags)
    : Share(parent, tr("Mission"), "mission", AppDirs::missions(), QStringList(), flags)
    , mission(mission)
{
    connect(this, &Share::imported, mission->storage, &MissionStorage::loadMission);
}

QString MissionShare::defaultExportFileName() const
{
    QString fname = mission->site().replace(" ", "");
    if (!fname.isEmpty())
        fname.append("-");
    fname.append(mission->vehicle->title());
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
