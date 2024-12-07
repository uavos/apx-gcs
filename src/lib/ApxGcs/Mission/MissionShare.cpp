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
#include "UnitMission.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Fleet/Unit.h>

MissionShare::MissionShare(UnitMission *mission, Fact *parent, Flags flags)
    : Share(parent, "mission", tr("Mission"), AppDirs::missions(), flags)
    , _mission(mission)
{
    connect(mission, &UnitMission::emptyChanged, this, &MissionShare::updateActions);
    updateActions();
}

QString MissionShare::getDefaultTitle()
{
    QString s = _mission->site().replace(" ", "");
    if (!s.isEmpty())
        s.append("-");
    s.append(_mission->unit->title());
    QString subj = _mission->f_title->text().simplified();
    if (!subj.isEmpty())
        s.append(QString("-%1").arg(subj));
    else
        s.append(QString("-%1").arg(_mission->missionSize()));
    s.replace(' ', '-');
    return s;
}
bool MissionShare::exportRequest(QString format, QString fileName)
{
    if (!saveData(_mission->toJsonDocument().toJson(), fileName))
        return false;
    _exported(fileName);
    return true;
}
bool MissionShare::importRequest(QStringList fileNames)
{
    auto fileName = fileNames.first();

    const auto jsv = Fact::parseJsonData(loadData(fileName));
    if (jsv.isNull())
        return false;

    _mission->fromJson(jsv);

    _imported(fileName, _mission->f_title->text());
    return true;
}

void MissionShare::updateActions()
{
    f_export->setEnabled(!_mission->empty());
}
