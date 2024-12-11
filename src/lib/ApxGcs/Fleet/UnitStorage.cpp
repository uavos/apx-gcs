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

#include <Database/DatabaseModel.h>
#include <Database/NodesReqUnit.h>

UnitStorage::UnitStorage(Unit *unit)
    : Fact(unit,
           "load",
           tr("Unit Parameters"),
           tr("Load unit parameters from database"),
           FilterModel | Action | IconOnly,
           "database-search")
    , _unit(unit)
{
    setOpt("page", "Menu/FactMenuPageLookupDB.qml");

    _dbmodel = new DatabaseModel(this);
    setModel(_dbmodel);

    connect(_dbmodel, &DatabaseModel::requestRecordsList, this, &UnitStorage::dbRequestRecordsList);
    connect(_dbmodel, &DatabaseModel::requestRecordInfo, this, &UnitStorage::dbRequestRecordInfo);
    connect(_dbmodel, &DatabaseModel::itemTriggered, this, &UnitStorage::dbRecordTriggered);

    connect(this, &Fact::triggered, this, &UnitStorage::dbRequestRecordsList);
}

void UnitStorage::saveUnitInfo()
{
    auto unit = _unit->get_info();
    if (unit.isEmpty())
        return;

    auto req = new db::nodes::UnitSaveInfo(unit);
    req->exec();
}

void UnitStorage::saveUnitConf()
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
        auto nconfID = i->tools->f_storage->configID();
        if (!nconfID)
            return;
        nconfIDs.append(nconfID);
    }
    auto vuid = _unit->uid();
    auto title = nodes->getConfigTitle();

    auto req = new db::nodes::UnitSaveConf(vuid, nconfIDs, title);
    connect(req,
            &db::nodes::UnitSaveConf::confSaved,
            this,
            &UnitStorage::confSaved,
            Qt::QueuedConnection);
    req->exec();
}

void UnitStorage::confLoaded(QJsonObject config)
{
    auto title = config.value("title").toString();

    QElapsedTimer timer;
    timer.start();

    _unit->fromJson(config);

    qDebug() << title << "loaded in" << timer.elapsed() << "ms";
    _unit->message(tr("Unit configuration loaded").append(": ").append(title));
}

void UnitStorage::importUnitConf(QJsonObject conf)
{
    auto req = new db::nodes::UnitImportConf(conf);
    req->exec();
}

void UnitStorage::dbRequestRecordsList()
{
    QStringList fields = {"Unit.name", "Unit.type", "UnitConf.title", "UnitConf.notes"};
    auto filter = _dbmodel->getFilterExpression(fields);

    QString s = "SELECT UnitConf.key, UnitConf.time FROM UnitConf"
                " LEFT JOIN Unit ON UnitConf.unitID=Unit.key";
    if (!filter.isEmpty())
        s += " WHERE " + filter;
    s += " ORDER BY UnitConf.time DESC";

    auto req = new db::nodes::Request(s, {});
    connect(req,
            &db::nodes::Request::queryResults,
            _dbmodel,
            &DatabaseModel::dbUpdateRecords,
            Qt::QueuedConnection);
    req->exec();
}

void UnitStorage::dbRequestRecordInfo(quint64 id)
{
    const QString s = "SELECT * FROM UnitConf"
                      " LEFT JOIN Unit ON UnitConf.unitID=Unit.key"
                      " WHERE UnitConf.key=?";

    auto req = new db::nodes::Request(s, {id});
    connect(
        req,
        &db::nodes::Request::queryResults,
        this,
        [this, id](QJsonArray records) {
            const auto jso = records.first().toObject();

            auto time = QDateTime::fromMSecsSinceEpoch(jso.value("time").toVariant().toULongLong())
                            .toString("yyyy MMM dd hh:mm:ss");
            auto callsign = jso.value("name").toString();
            auto title = jso.value("title").toString();
            auto notes = jso.value("notes").toString();

            QJsonObject info;
            info.insert("title", time);
            info.insert("value", title);
            info.insert("descr", callsign + (notes.isEmpty() ? "" : QString(" - %1").arg(notes)));

            _dbmodel->setRecordModelInfo(id, info);
        },
        Qt::QueuedConnection);
    req->exec();
}

void UnitStorage::dbRecordTriggered(quint64 id)
{
    auto req = new db::nodes::UnitLoadConf(id);
    connect(req,
            &db::nodes::UnitLoadConf::confLoaded,
            this,
            &UnitStorage::confLoaded,
            Qt::QueuedConnection);
    req->exec();
}
