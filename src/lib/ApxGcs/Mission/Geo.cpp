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
#include "Geo.h"
#include "MissionField.h"
#include "UnitMission.h"
#include <App/App.h>
#include <QGeoCircle>

Geo::Geo(MissionGroup *parent)
    : MissionItem(parent, "g#", "", "")
{
    setOpt("color", "#E65100");

    f_role = new Fact(this, "role", tr("Role"), "", Fact::Enum);
    f_role->setEnumStrings({
        "safe",
        "nofly",
        "terminate",
        "auxiliary",
    });

    f_shape = new Fact(this, "shape", tr("Shape"), "", Fact::Enum);
    f_shape->setEnumStrings({
        "circle",
        "polygon",
        "line",
    });

    f_label = new Fact(this, "label", tr("Label"), tr("Geofence label"), Fact::Text);

    f_top = new MissionField(this, "top", tr("Top"), tr("Top altitude AMSL"), Fact::Int);
    f_top->setUnits("m");
    f_top->setMin(0);
    f_top->setIncrement(100);
    f_top->setEnumStrings({
        tr("unlimited"),
    });

    f_bottom = new MissionField(this, "bottom", tr("Bottom"), tr("Bottom altitude AMSL"), Fact::Int);
    f_bottom->setUnits("m");
    f_bottom->setMin(0);
    f_bottom->setIncrement(100);
    f_bottom->setEnumStrings({
        tr("ground"),
    });

    f_inverted = new Fact(this, "inverted", tr("Inverted"), tr("Valid when outside"), Fact::Bool);

    f_points = new Fact(this,
                        "points",
                        tr("Points"),
                        tr("Geofence points"),
                        Fact::Group | Fact::ModifiedGroup | Fact::Count);

    auto f_remove = new Fact(this, "remove", tr("Remove"), tr("Remove geofence"), Action | Remove);
    connect(f_remove, &Fact::triggered, this, &Fact::deleteFact);

    //title
    updateTitle();
    for (auto f : facts())
        connect(f, &Fact::valueChanged, this, &Geo::updateTitle);

    App::jsync(this);
}

void Geo::updateTitle()
{
    QStringList st;

    st.append(f_role->valueText());

    if (f_inverted->value().toBool()) {
        st.append(tr("inverted"));
    }
    st.append(f_shape->valueText());

    auto label = f_label->valueText();
    if (!label.isEmpty())
        st.append("\"" + label + "\"");

    auto top = f_top->value().toInt();
    auto bottom = f_bottom->value().toInt();
    if (bottom != 0 && top != 0) {
        st.append(QString("%1-%2").arg(bottom).arg(top));
    } else if (bottom != 0) {
        st.append(QString("%1+").arg(bottom));
    } else if (top != 0) {
        st.append(QString("0-%1").arg(top));
    }

    auto sz = f_points->size();
    if (sz > 0)
        st.append(QString("[%1]").arg(sz));

    setTitle(st.join(' '));
}
void Geo::updateDescr()
{
    QStringList st;
    QString sts;
    setDescr(st.join(' '));
    setValue(sts);
}

QGeoRectangle Geo::boundingGeoRectangle() const
{
    return MissionItem::boundingGeoRectangle();
    //.united(
    //    QGeoCircle(coordinate(), std::abs(f_radius->value().toDouble())).boundingGeoRectangle());
}
