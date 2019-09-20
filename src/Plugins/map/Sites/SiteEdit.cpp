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
#include "SiteEdit.h"
#include <ApxApp.h>
//=============================================================================
SiteEdit::SiteEdit(Fact *parent,
                   const QString &name,
                   const QString &title,
                   const QString &descr,
                   QVariantMap modelData)
    : Fact(parent, name, title, descr, Group)
    , modelData(modelData)
    , blockUpdateItemData(false)
{
    f_title = new Fact(this, "sname", tr("Site name"), tr("Label for geographic area"), Text);
    f_latitude = new Fact(this, "latitude", tr("Latitude"), tr("Global postition latitude"), Float);
    f_latitude->setUnits("lat");
    f_longitude = new Fact(this,
                           "longitude",
                           tr("Longitude"),
                           tr("Global postition longitude"),
                           Float);
    f_longitude->setUnits("lon");

    if (!modelData.isEmpty()) {
        f_missions = new LookupMissions(nullptr, this, Action);
        connect(f_missions, &Fact::triggered, this, &SiteEdit::lookupMissions);

        a_remove = new Fact(this,
                            "remove",
                            tr("Remove"),
                            "",
                            Action | Remove | CloseOnTrigger | ShowDisabled);
        connect(a_remove, &Fact::triggered, this, [this]() {
            emit removeTriggered(this->modelData);
        });
    } else {
        a_add = new Fact(this, "add", tr("Add"), "", Action | Apply | CloseOnTrigger | ShowDisabled);
        a_add->setEnabled(false);
        connect(a_add, &Fact::triggered, this, [this]() { emit addTriggered(this->modelData); });
        connect(f_title, &Fact::textChanged, this, [=]() {
            a_add->setEnabled(!f_title->text().isEmpty());
        });
        connect(this, &Fact::triggered, this, &SiteEdit::reset);
        reset();
    }

    loadFromModelData();

    connect(f_title, &Fact::valueChanged, this, &SiteEdit::saveToModelData);
    connect(f_latitude, &Fact::valueChanged, this, &SiteEdit::saveToModelData);
    connect(f_longitude, &Fact::valueChanged, this, &SiteEdit::saveToModelData);
}
//=============================================================================
void SiteEdit::reset()
{
    modelData.remove("title");
    QGeoCoordinate c(ApxApp::propertyValue<QGeoCoordinate>("apx.tools.MapView.clickCoordinate"));
    modelData["lat"] = c.latitude();
    modelData["lon"] = c.longitude();
    loadFromModelData();
}
//=============================================================================
void SiteEdit::loadFromModelData()
{
    blockUpdateItemData = true;
    f_title->setValue(modelData.value("title").toString());
    f_latitude->setValue(modelData.value("lat").toDouble());
    f_longitude->setValue(modelData.value("lon").toDouble());
    blockUpdateItemData = false;
}
void SiteEdit::saveToModelData()
{
    if (blockUpdateItemData)
        return;
    if (f_title->text().isEmpty()) {
        loadFromModelData();
        return;
    }
    double lat = f_latitude->value().toDouble();
    double lon = f_longitude->value().toDouble();
    if (std::isnan(lat) || std::isnan(lon) || lat == 0.0 || lon == 0.0) {
        loadFromModelData();
        return;
    }
    modelData["title"] = f_title->text();
    modelData["lat"] = lat;
    modelData["lon"] = lon;
    emit siteEdited(modelData);
}
void SiteEdit::setModelData(QVariantMap v)
{
    if (modelData.value("key").toULongLong() != v.value("key").toULongLong())
        return;
    modelData = v;
    loadFromModelData();
}
void SiteEdit::updateFromEditedModelData(int i, QVariantMap v)
{
    Q_UNUSED(i)
    setModelData(v);
    emit siteEdited(modelData);
}
//=============================================================================
void SiteEdit::lookupMissions()
{
    QGeoCoordinate c(modelData.value("lat").toDouble(), modelData.value("lon").toDouble());
    f_missions->dbLookupMissionsByArea(c, modelData.value("title").toString());
}
//=============================================================================
//=============================================================================
