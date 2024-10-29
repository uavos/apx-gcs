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
#include "Sites.h"
#include <App/App.h>
#include <Database/MissionsDB.h>

Sites::Sites(Fact *parent)
    : Fact(parent, QString(PLUGIN_NAME).toLower(), tr("Sites"), tr("Geographic objects"), Group)
    , f_edit(nullptr)
{
    setIcon("city");
    connect(App::instance(), &App::loadingFinished, this, &Sites::appLoaded);

    f_lookup = new LookupSites(this);

    //facts
    f_add = new SiteEdit(this, "add", tr("Add new site"), tr("Create new named area"), QVariantMap());
    f_add->setIcon("plus-circle");
    connect(f_add, &SiteEdit::addTriggered, this, &Sites::dbAddSite);

    //App::jsync(this);

    loadQml("qrc:/" PLUGIN_NAME "/SitesPlugin.qml");
}

void Sites::appLoaded()
{
    //qDebug()<<"appLoaded";
    //add menus to map tools plugin
    Fact *fMapAdd = AppRoot::instance()->findChild("tools.missionplanner.add");
    if (!fMapAdd)
        return;
    //create tool for map
    SiteEdit *f = new SiteEdit(fMapAdd, "site", tr("Site"), "", QVariantMap());
    f->setIcon("city");
    connect(f, &SiteEdit::addTriggered, f_add, &SiteEdit::addTriggered);
}

void Sites::createEditor(QVariantMap item)
{
    //qDebug()<<item.value("title").toString();
    if (f_edit)
        f_edit->deleteFact();
    f_edit = new SiteEdit(this, "edit", tr("Edit site"), tr("Edit area parameters"), item);
    f_edit->setIcon("cog");
    connect(f_edit, &SiteEdit::removed, this, [this]() { f_edit = nullptr; });
    connect(f_edit, &SiteEdit::removeTriggered, this, &Sites::dbRemoveSite);
    connect(f_edit, &SiteEdit::siteEdited, this, &Sites::dbUpdateSite);
    connect(f_lookup->dbModel(),
            &DatabaseLookupModel::itemEdited,
            f_edit,
            &SiteEdit::updateFromEditedModelData);
    connect(f_lookup->dbModel(), &DatabaseLookupModel::synced, this, &Sites::syncEditorFromModel);
}
void Sites::destroyEditor(QVariantMap item)
{
    //qDebug()<<item.value("title").toString();
    Q_UNUSED(item)
    if (!f_edit)
        return;
    if (f_edit->modelData.value("key").toULongLong() != item.value("key").toULongLong())
        return;
    f_edit->deleteFact();
    f_edit = nullptr;
}
void Sites::syncEditorFromModel()
{
    if (!f_edit)
        return;
    int i = f_lookup->dbModel()->indexOf("key", f_edit->modelData.value("key"));
    if (i < 0) {
        destroyEditor(f_edit->modelData);
        return;
    }
    f_edit->setModelData(f_lookup->dbModel()->get(i));
}

void Sites::dbAddSite(QVariantMap item)
{
    //qDebug()<<item;
    DBReqMissionsSaveSite *req = new DBReqMissionsSaveSite(item.value("title").toString(),
                                                           item.value("lat").toDouble(),
                                                           item.value("lon").toDouble());
    connect(req,
            &DatabaseRequest::finished,
            f_lookup,
            &DatabaseLookup::defaultLookup,
            Qt::QueuedConnection);
    connect(
        req,
        &DBReqMissionsSaveSite::siteAdded,
        this,
        [](QString title) { apxMsg() << tr("Site added").append(':') << title; },
        Qt::QueuedConnection);
    req->exec();
}

void Sites::dbRemoveSite(QVariantMap item)
{
    destroyEditor(item);
    //qDebug()<<item;
    quint64 key = item.value("key").toULongLong();
    if (!key)
        return;
    DBReqMissionsRemoveSite *req = new DBReqMissionsRemoveSite(key);
    connect(req,
            &DatabaseRequest::finished,
            f_lookup,
            &DatabaseLookup::defaultLookup,
            Qt::QueuedConnection);
    connect(
        req,
        &DBReqMissionsRemoveSite::siteRemoved,
        this,
        []() { apxMsg() << tr("Site removed"); },
        Qt::QueuedConnection);
    req->exec();
}

void Sites::dbUpdateSite(QVariantMap item)
{
    //qDebug()<<item;
    quint64 key = item.value("key").toULongLong();
    if (!key)
        return;
    DBReqMissionsSaveSite *req = new DBReqMissionsSaveSite(item.value("title").toString(),
                                                           item.value("lat").toDouble(),
                                                           item.value("lon").toDouble(),
                                                           key);
    connect(req,
            &DatabaseRequest::finished,
            f_lookup,
            &DatabaseLookup::defaultLookup,
            Qt::QueuedConnection);
    connect(
        req,
        &DBReqMissionsSaveSite::siteModified,
        this,
        [](QString title) { apxMsg() << tr("Site updated").append(':') << title; },
        Qt::QueuedConnection);
    req->exec();
}
