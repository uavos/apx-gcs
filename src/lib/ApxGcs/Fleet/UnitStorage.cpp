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
#include "UnitStorage.h"
#include "Unit.h"

#include <Nodes/Nodes.h>

#include <Database/FleetReqUnit.h>

UnitStorage::UnitStorage(Unit *unit)
    : QObject(unit)
    , _unit(unit)
{}

void UnitStorage::saveUnitInfo()
{
    auto m = _unit->get_info();
    if (m.isEmpty())
        return;

    auto *req = new DBReqSaveUnitInfo(m.toVariantMap());
    req->exec();
}

void UnitStorage::saveUnitConfig()
{
    auto nodes = _unit->f_nodes;

    if (!nodes->valid())
        return;

    if (nodes->nodes().isEmpty())
        return;

    QList<quint64> nconfIDs;
    for (auto i : nodes->nodes()) {
        if (!i->valid())
            return;
        auto nconfID = i->storage->configID();
        if (!nconfID)
            return;
        nconfIDs.append(nconfID);
    }
    auto vuid = _unit->uid();
    auto title = nodes->getConfigTitle();

    auto *req = new DBReqSaveUnitConfig(vuid, nconfIDs, title);
    connect(req,
            &DBReqSaveUnitConfig::configSaved,
            this,
            &UnitStorage::configSaved,
            Qt::QueuedConnection);
    req->exec();
}

void UnitStorage::loadUnitConfig(QString hash)
{
    auto *req = new DBReqLoadUnitConfig(hash);
    connect(req,
            &DBReqLoadUnitConfig::configLoaded,
            this,
            &UnitStorage::configLoaded,
            Qt::QueuedConnection);
    req->exec();
}
void UnitStorage::configLoaded(QVariantMap config)
{
    auto title = config.value("title").toString();

    QElapsedTimer timer;
    timer.start();

    _unit->fromVariant(config);

    qDebug() << title << "loaded in" << timer.elapsed() << "ms";
    _unit->message(tr("Unit configuration loaded").append(": ").append(title));
}

void UnitStorage::importUnitConfig(QVariantMap config)
{
    auto *req = new DBReqImportUnitConfig(config);
    req->exec();
}
