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

#include "NearestSites.h"
#include "SiteEdit.h"
#include <Fact/Fact.h>
#include <QtCore>
#include <QtLocation>

#include <ApxMisc/DelayedEvent.h>

class Unit;

class Sites : public Fact
{
    Q_OBJECT

    Q_PROPERTY(Fact *editor READ editor CONSTANT)

public:
    explicit Sites(Fact *parent = nullptr);

    auto editor() const { return f_edit; }

    Q_INVOKABLE void createEditor(QVariantMap item);
    Q_INVOKABLE void destroyEditor(QVariantMap item);
    Q_INVOKABLE void updateSite(QVariantMap item) { dbUpdateSite(item); }

    NearestSites *f_nearest;

private:
    SiteEdit *f_add;
    SiteEdit *f_edit;

    DelayedEvent evtUpdateMissionSite;

private slots:
    void appLoaded();

    void dbAddSite(QVariantMap item);
    void dbRemoveSite(QVariantMap item);
    void dbUpdateSite(QVariantMap item);
    void dbFindSite();
};
