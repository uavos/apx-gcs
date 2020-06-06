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
#include "Firmware.h"
#include "FirmwareTools.h"
#include "LoaderStm.h"
#include "QueueItem.h"
#include "Releases.h"

#include <App/AppGcs.h>
#include <App/AppLog.h>

APX_LOGGING_CATEGORY(FirmwareLog, "core.Firmware")

Firmware *Firmware::_instance = nullptr;
Firmware::Firmware(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Firmware"),
           tr("Firmware updates"),
           Group | FlatModel | ProgressTrack,
           "update")
{
    _instance = this;

    setOpt("pos", QPointF(1, 1));

    f_releases = new Releases(this);

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

    f_start = new Fact(this,
                       "start",
                       tr("Upgrade"),
                       tr("Auto upgrade all nodes"),
                       Action,
                       "auto-upload");

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop upgrading"), Action | Stop);
    connect(f_stop, &Fact::triggered, nodes_protocol(), []() {
        AppGcs::instance()->protocol->stopNmtRequests();
    });

    connect(nodes_protocol(), &Fact::activeChanged, this, [this]() {
        if (!nodes_protocol()->active()) {
            setActive(false);
            f_queue->removeAll();
        }
    });
    connect(this, &Fact::activeChanged, nodes_protocol(), [this]() {
        nodes_protocol()->setUpgrading(active());
    });

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

    connect(AppGcs::instance()->protocol,
            &ProtocolVehicles::nodeNotify,
            this,
            &Firmware::nodeNotify);
}

ProtocolNodes *Firmware::nodes_protocol()
{
    return AppGcs::instance()->protocol->local->nodes;
}

void Firmware::nodeNotify(ProtocolNode *protocol)
{
    if (!protocol->identValid())
        return;

    connect(protocol,
            &ProtocolNode::requestUpgrade,
            this,
            &Firmware::requestUpgrade,
            Qt::UniqueConnection);

    QString sn = protocol->sn();

    if (queued(f_queue, sn))
        return;
    if (queued(f_available, sn))
        return;

    // FIXME: queue for auto update
    new QueueItem(f_available, protocol, QString());
}

void Firmware::updateStatus()
{
    bool act = active();

    //actions
    f_stop->setEnabled(act);
    f_start->setEnabled(!act && f_available->size() > 0);
    if (act) {
        setValue(QString("[%1]").arg(f_queue->size()));
    } else {
        setValue("");
    }
}

QueueItem *Firmware::queued(Fact *list, const QString &sn)
{
    for (auto i : list->facts()) {
        QueueItem *f = static_cast<QueueItem *>(i);
        if (f->match(sn))
            return f;
    }
    return nullptr;
}

void Firmware::requestUpgrade(ProtocolNode *protocol, QString type)
{
    requestFormat(protocol, type, QString(), QString());
}
void Firmware::requestInitialize(const QString &type,
                                 const QString &name,
                                 const QString &hw,
                                 const QString &portName,
                                 bool continuous)
{
    setActive(false);
    f_queue->removeAll();

    new LoaderStm(f_queue, type, name, hw, portName, continuous);
}
void Firmware::requestFormat(ProtocolNode *protocol, QString type, QString name, QString hw)
{
    QString sn = protocol->sn();
    qDebug() << sn << type << protocol->title();

    QueueItem *f = queued(f_available, sn);
    if (f)
        f->remove();

    f = queued(f_queue, sn);
    if (f)
        f->remove();

    QueueItem *q = new QueueItem(f_queue, protocol, type);
    q->format_name = name;
    q->format_hw = hw;

    if (!active())
        next();
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
    nodes_protocol()->setActive(true);
    emit upgradeStarted(item->protocol() ? item->protocol()->sn() : QString(), item->type());

    connect(item, &QueueItem::finished, this, &Firmware::loaderFinished);

    item->move(0);
    item->start();
}

void Firmware::loaderFinished(QueueItem *item, bool success)
{
    emit upgradeFinished(item->protocol() ? item->protocol()->sn() : QString(), item->type());

    if (success) {
        item->remove();
    } else {
        f_queue->removeAll();
    }

    if (f_queue->size() == 0) {
        nodes_protocol()->setActive(false);
        setActive(false);
    }

    QTimer::singleShot(100, this, &Firmware::next);
}
