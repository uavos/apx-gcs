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
#include <Vehicles/Vehicle.h>

#include <Sharing/ShareExport.h>

#include <Database/MissionsDB.h>

MissionShare::MissionShare(VehicleMission *mission, Fact *parent, Flags flags)
    : Share(parent, tr("Mission"), AppDirs::missions(), flags)
    , mission(mission)
{
    connect(this,
            &Share::imported,
            mission->storage,
            [mission](QString fileName, QString hash, QString title) {
                mission->storage->loadMission(hash);
            });

    add(new ShareExport("mission", "mission", this));
}

QString MissionShare::getDefaultTitle()
{
    QString s = mission->site().replace(" ", "");
    if (!s.isEmpty())
        s.append("-");
    s.append(mission->vehicle->title());
    QString subj = mission->f_title->text().simplified();
    if (!subj.isEmpty())
        s.append(QString("-%1").arg(subj));
    else
        s.append(QString("-%1").arg(mission->missionSize()));
    s.replace(' ', '-');
    return s;
}
bool MissionShare::exportRequest(ShareExport *format, QString fileName)
{
    return format->saveData(mission->toJsonDocument().toJson(), fileName);
}
bool MissionShare::importRequest(ShareImport *format, QString fileName)
{
    //auto f = qobject_cast<MissionImport *>(format);
    //return f->load(fileName);
}
