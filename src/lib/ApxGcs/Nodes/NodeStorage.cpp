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
#include "NodeStorage.h"
#include "NodeItem.h"

#include <Database/FleetReqNode.h>

#include <Database/NodesReqConf.h>
#include <Database/NodesReqDict.h>
#include <Database/NodesReqMeta.h>
#include <Database/NodesReqUnit.h>

NodeStorage::NodeStorage(NodeItem *node)
    : QObject(node)
    , _node(node)
{}

void NodeStorage::saveNodeInfo()
{
    auto info = _node->ident();
    if (info.isEmpty())
        return;

    info["time"] = QDateTime::currentDateTime().toMSecsSinceEpoch();

    auto req = new db::nodes::NodeSaveInfo(info);
    req->exec();
}

void NodeStorage::saveNodeDict()
{
    auto dict = _node->get_dict();
    if (dict.isEmpty()) {
        qWarning() << "no dict data";
        return;
    }

    auto req = new DBReqSaveNodeDict(_node->uid(), dict);
    req->exec();
}

void NodeStorage::saveNodeConfig()
{
    _configID = 0; // invalidate for unit config
    auto hash = _node->get_dict().value("hash").toString();
    if (hash.isEmpty()) {
        qWarning() << "no dict hash";
        return;
    }
    auto req = new DBReqSaveNodeConfig(_node->uid(), hash, _node->get_values());
    connect(req,
            &DBReqSaveNodeConfig::configSaved,
            this,
            &NodeStorage::updateConfigID,
            Qt::QueuedConnection);
    req->exec();
}
void NodeStorage::updateConfigID(quint64 configID)
{
    _configID = configID;
    if (configID)
        emit configSaved();
}

void NodeStorage::loadNodeConfig(QString hash)
{
    auto req = new DBReqLoadNodeConfig(_node->uid(), hash);
    connect(req,
            &DBReqLoadNodeConfig::configLoaded,
            _node,
            &NodeItem::importValues,
            Qt::QueuedConnection);
    req->exec();
}

void NodeStorage::loadNodeMeta()
{
    auto req = new db::nodes::LoadFieldMeta(get_names(_node));
    connect(req,
            &db::nodes::LoadFieldMeta::dictMetaLoaded,
            this,
            &NodeStorage::dictMetaLoaded,
            Qt::QueuedConnection);
    req->exec();
}
QStringList NodeStorage::get_names(Fact *f, QStringList path)
{
    QStringList names;
    for (auto i : f->facts()) {
        auto p = path;
        p << i->name();
        names.append(p.join('.'));
        names.append(get_names(i, p));
    }
    return names;
}
void NodeStorage::dictMetaLoaded(QJsonObject jso)
{
    QElapsedTimer timer;
    timer.start();

    uint cnt = 0;
    for (auto it = jso.begin(); it != jso.end(); ++it) {
        const auto name = it.key();
        const auto meta = it.value().toObject();

        auto f = _node->findChild(name);
        if (!f)
            continue;

        cnt++;

        auto descr = meta["descr"].toString();

        descr = descr.left(descr.indexOf('\n'));
        descr = descr.left(descr.indexOf('.'));

        f->setDescr(descr.simplified());

        NodeField *nf = qobject_cast<NodeField *>(f);
        if (!nf)
            continue;

        auto def = meta["def"];
        auto min = meta["min"];
        auto max = meta["max"];
        auto increment = meta["increment"];
        auto decimal = meta["decimal"];

        if (!def.isNull())
            nf->setDefaultValue(def);
        if (!min.isNull())
            nf->setMin(min);
        if (!max.isNull())
            nf->setMax(max);
        if (!increment.isNull())
            nf->setIncrement(increment.toDouble());
        // if (!decimal.isNull())
        //     nf->setPrecision(decimal.toInt());
    }
    // if (cnt > 0)
    //     qDebug() << "meta data loaded:" << cnt << "fields in" << timer.elapsed() << "ms";
}
