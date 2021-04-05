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

    setValue(tr("Initializing update").append("..."));
    upload();
}

bool QueueItem::loadFirmware(QString hw, QString ver)
{
    QString stype = type().toUpper();

    ApxFw *apxfw = AppGcs::apxfw();
    QString relVer = apxfw->value().toString();
    if (ver != relVer) {
        ver = QString("%1->%2").arg(ver).arg(relVer);
    }

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

void QueueItem::upload()
{
    // TODO: upload a node file
    /*cleanUploadConnections();
    Firmware::nodes_protocol()->clear_requests();

    if (!loadFirmware(protocol()->hardware(), protocol()->version())) {
        finish(false);
        return;
    }
    PNodeFile *f = file(type());
    if (!f) {
        finish(false);
        return;
    }
    f->upload(_data, _offset);*/
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

/*PNodeFile *QueueItem::file(const QString &fname)
{
    PNodeFile *file = protocol()->file(fname);
    if (!file) {
        AppNotify::instance()
            ->report(QString("%1: %2/%3").arg(tr("Node file is unavailable")).arg(title()).arg(fname),
                     AppNotify::FromApp | AppNotify::Error);
        return nullptr;
    }

    if (file_p) {
        disconnect(file_p, nullptr, this, nullptr);
    }
    file_p = file;

    connect(file_p, &PNodeFile::uploaded, this, [this]() { finish(true); });
    connect(file_p, &PNodeFile::error, this, [this]() { finish(false); });
    connect(file_p, &PNodeFile::interrupted, this, [this]() { finish(false); });

    connect(file_p, &PNodeFile::valueChanged, this, [this]() { setValue(file_p->value()); });

    return file;
}*/
