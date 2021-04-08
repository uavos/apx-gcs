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
#include "QueueItem.h"
#include "Firmware.h"

#include <App/AppGcs.h>
#include <App/AppNotify.h>

QueueItem::QueueItem(Fact *parent, QString uid, QString name, QString hw, QString type)
    : Fact(parent, "item#", QString("%1:%2").arg(name).arg(hw))
    , _uid(uid)
    , _name(name)
    , _hw(hw)
{
    setTreeType(NoFlags);
    setType(type);
}

bool QueueItem::match(const QString &uid) const
{
    return _uid == uid;
}
void QueueItem::setType(QString v)
{
    _type = v;
    setDescr(_type.toUpper());
}

void QueueItem::start()
{
    qDebug() << title();

    //setValue(tr("Initializing update").append("..."));
    upload();
}

bool QueueItem::loadFirmware(QString hw)
{
    QString stype = type().toUpper();

    ApxFw *apxfw = AppGcs::apxfw();
    QString ver = apxfw->value().toString();

    QString s = QString("%1 %2 (%3)").arg(_name).arg(_hw).arg(ver);
    s = QString("%1 (%2): %3").arg(tr("Firmware upload")).arg(stype).arg(s);
    AppNotify::instance()->report(s, AppNotify::FromApp | AppNotify::Important);

    _data.clear();
    _offset = 0;

    QString rel_type = type();
    if (rel_type == "fw")
        rel_type = "firmware";
    else if (rel_type == "ldr")
        rel_type = "loader";
    else
        apxMsgW() << "unknown type:" << type();

    return apxfw->loadFirmware(_name, _hw, rel_type, &_data, &_offset);
}

PFirmware *QueueItem::protocol() const
{
    auto p = AppGcs::instance()->f_datalink->f_protocols->current();
    if (!p)
        return {};
    return p->firmware();
}

void QueueItem::upload()
{
    if (!loadFirmware(_hw)) {
        finish(false);
        return;
    }
    auto p = protocol();
    if (!p) {
        finish(false);
        return;
    }
    bindProperty(p, "progress", true);
    bindProperty(p, "value", true);
    p->upgradeFirmware(_uid, _type, _data, _offset);
    connect(p, &PFirmware::upgradeFinished, this, &QueueItem::upgradeFinished);
}

void QueueItem::upgradeFinished(QString uid, bool success)
{
    disconnect(protocol(), nullptr, this, nullptr);
    finish(success);
}

void QueueItem::finish(bool success)
{
    if (success) {
        AppNotify::instance()->report(tr("Firmware upload finished").append(": ").append(title()),
                                      AppNotify::FromApp | AppNotify::Important);
    } else {
        AppNotify::instance()->report(tr("Firmware upload error").append(": ").append(title()),
                                      AppNotify::FromApp | AppNotify::Error);
    }

    //qDebug() << success;
    emit finished(this, success);
}
