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
#include "AirspaceItem.h"
#include "MissionField.h"
#include "UnitMission.h"
#include <App/App.h>
#include <QGeoCircle>

AirspaceItem::AirspaceItem(MissionGroup *parent)
    : MissionItem(parent, "A#", "", tr("Geofence"))
{
    setOpt("color", "#E65100");

    f_hmsl = new MissionField(this, "hmsl", tr("HMSL"), tr("Object of interest altitude MSL"), Int);
    f_hmsl->setUnits("m");
    f_hmsl->setEnumStrings(QStringList() << "ground");

    //title
    updateTitle();

    connect(f_hmsl, &Fact::valueChanged, this, &AirspaceItem::updateDescr);
    updateDescr();

    App::jsync(this);
}

void AirspaceItem::updateTitle()
{
    QStringList st;
    st.append(QString::number(num() + 1));
    setTitle(st.join(' '));
}
void AirspaceItem::updateDescr()
{
    QStringList st;
    QString sts;
    if (!f_hmsl->isZero()) {
        st.append("MSL" + f_hmsl->valueText());
        sts.append("H");
    }
    setDescr(st.join(' '));
    setValue(sts);
}
