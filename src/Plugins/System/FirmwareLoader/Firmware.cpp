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
#include "Firmware.h"
#include "FirmwareTools.h"
#include "LoaderStm.h"
#include "QueueItem.h"

#include <App/AppGcs.h>
#include <App/AppLog.h>

APX_LOGGING_CATEGORY(FirmwareLog, "core.Firmware")

Firmware *Firmware::_instance = nullptr;
Firmware::Firmware(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Firmware loader"),
           tr("Devices update tool"),
           Group | FlatModel | ProgressTrack,
           "update")
{
    _instance = this;

    setOpt("pos", QPointF(1, 1));

    f_queue = new Fact(this,
                       "queue",
                       tr("Queue"),
                       tr("Nodes to upgrade"),
                       Section | Count | ProgressTrack);
    f_queue->setIcon("playlist-play");

    f_available = new Fact(this,
                           "available",
                           tr("Available"),
                           tr("Upgradable nodes"),
                           Section | Count);
    f_available->setIcon("star-circle");

    // TODO auto upgrade all nodes
    /*f_start = new Fact(this,
                       "start",
                       tr("Upgrade"),
                       tr("Auto upgrade all nodes"),
                       Action,
                       "auto-upload");*/

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop upgrading"), Action | Stop);
    connect(f_stop, &Fact::triggered, this, &Firmware::stop);

    connect(Fleet::instance(), &Fleet::unitRegistered, this, &Firmware::unitRegistered);

    connect(
        f_queue,
        &Fact::sizeChanged,
        this,
        [this]() {
            if (f_queue->size() == 0)
                return;
            if (f_queue->size() == 1 && !active())
                next();
            trigger(); // show menu
        },
        Qt::QueuedConnection);

    //tools actions
    f_tools = new FirmwareTools(this);

    connect(f_queue, &Fact::sizeChanged, this, &Firmware::updateStatus);
    connect(f_available, &Fact::sizeChanged, this, &Firmware::updateStatus);
    connect(this, &Fact::activeChanged, this, &Firmware::updateStatus);
    updateStatus();
}

void Firmware::unitRegistered(Unit *unit)
{
    connect(unit->f_nodes, &Nodes::requestUpgrade, this, &Firmware::upgradeRequested);
    connect(unit->f_nodes, &Nodes::nodeNotify, this, &Firmware::nodeNotify);
}
void Firmware::nodeNotify(NodeItem *node)
{
    auto uid = node->uid();

    // update nodes map
    QStringList st;
    st << node->title();
    st << node->descr();
    st << node->value().toString();
    st.removeAll("");
    QString s = st.join(' ');
    if (_nodesMap.value(uid) != s) {
        _nodesMap.insert(uid, s);
        nodesMapUpdated(_nodesMap);
    }

    if (node->ident().isEmpty())
        return;

    if (queued(f_queue, uid))
        return;
    if (queued(f_available, uid))
        return;

    new QueueItem(f_available,
                  node->uid(),
                  node->title(),
                  node->ident().value("hardware").toString(),
                  QString());
}

void Firmware::updateStatus()
{
    bool act = active();

    //actions
    f_stop->setEnabled(act);
    //f_start->setEnabled(!act && f_available->size() > 0);
    if (act) {
        setValue(QString("[%1]").arg(f_queue->size()));
    } else {
        setValue("");
    }
}

void Firmware::stop()
{
    QueueItem::protocol()->root()->cancelRequests();
    f_queue->deleteChildren();
    setActive(false);
    updateStatus();
}

QueueItem *Firmware::queued(Fact *list, const QString &uid)
{
    for (auto i : list->facts()) {
        QueueItem *f = static_cast<QueueItem *>(i);
        if (f->match(uid))
            return f;
    }
    return nullptr;
}

void Firmware::upgradeRequested(NodeItem *node, QString type)
{
    requestUpgrade(node->uid(), node->title(), node->ident().value("hardware").toString(), type);
}

void Firmware::requestUpgrade(QString uid, QString name, QString hw, QString type)
{
    qDebug() << uid << type << name;

    auto f = queued(f_available, uid);
    if (f)
        f->deleteFact();

    f = queued(f_queue, uid);
    if (f)
        f->deleteFact();

    new QueueItem(f_queue, uid, name, hw, type);

    if (!active())
        next();
}
void Firmware::requestInitialize(
    QString name, QString hw, QString type, QString portName, bool continuous)
{
    setActive(false);
    f_queue->deleteChildren();

    new LoaderStm(f_queue, name, hw, type, portName, continuous);
}

void Firmware::next()
{
    if (f_queue->size() <= 0) {
        setActive(false);
        return;
    }

    //find next node in queue
    QueueItem *item = nullptr;
    //loaders first
    for (int i = 0; item == nullptr && i < f_queue->size(); ++i) {
        QueueItem *f = static_cast<QueueItem *>(f_queue->child(i));
        if (f->type() == "ldr")
            item = f;
    }
    //sorted by names
    QStringList names;
    names << "servo"
          << "gps"
          << "cas"
          << "nav"
          << "ifc"
          << "";
    for (auto s : names) {
        for (int i = 0; item == nullptr && i < f_queue->size(); ++i) {
            QueueItem *f = static_cast<QueueItem *>(f_queue->child(i));
            if (s.isEmpty() || f->title() == s) {
                item = f;
            }
        }
        if (item)
            break;
    }
    if (!item)
        return;

    setActive(true);
    emit upgradeStarted(item->uid(), item->type());

    connect(item, &QueueItem::finished, this, &Firmware::itemFinished);

    item->move(0);
    item->start();
}

void Firmware::itemFinished(QueueItem *item, bool success)
{
    emit upgradeFinished(item->uid(), item->type());

    if (success) {
        item->deleteFact();
    } else {
        f_queue->deleteChildren();
    }

    if (f_queue->size() == 0) {
        setActive(false);
    }

    QTimer::singleShot(100, this, &Firmware::next);
}
