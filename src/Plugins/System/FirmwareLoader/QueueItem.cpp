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
#include "QueueItem.h"
#include "Firmware.h"
#include "Releases.h"

#include <App/AppGcs.h>
#include <App/AppNotify.h>

QueueItem::QueueItem(Fact *parent, ProtocolNode *protocol, QString type)
    : ProtocolViewBase(parent, protocol)
    , m_type(type)
{
    setTreeType(NoFlags);
    //setTitle(QString("%1 (%2)").arg(title()).arg(type));
    /*if (type == Firmware::LD)
        setIcon("alert-circle");
    else
        setIcon("chip");*/
}

bool QueueItem::match(const QString &sn) const
{
    return protocol() && protocol()->sn() == sn;
}
QString QueueItem::type() const
{
    return m_type;
}
void QueueItem::setType(QString v)
{
    m_type = v;
}

void QueueItem::start()
{
    qDebug() << title();

    Firmware::nodes_protocol()->clear_requests();

    setValue(tr("Initializing update").append("..."));

    if (!protocol()) {
        upload();
        return;
    }

    // always refresh ident
    protocol()->setIdentValid(false);
    if (type() == "fw") {
        _clist.append(connect(protocol(), &ProtocolNode::loaderAvailable, this, &QueueItem::upload));
        protocol()->requestRebootLoader();
        return;
    }

    _clist.append(connect(protocol(),
                          &ProtocolNode::filesAvailable,
                          this,
                          &QueueItem::upload,
                          Qt::UniqueConnection));
    connect(Firmware::nodes_protocol(),
            &Fact::activeChanged,
            this,
            &QueueItem::cleanUploadConnections,
            Qt::UniqueConnection);
    protocol()->requestIdent();
}
void QueueItem::cleanUploadConnections()
{
    for (auto c : _clist)
        disconnect(c);
    _clist.clear();
}

bool QueueItem::loadFirmware(QString hw, QString ver)
{
    QString fw = title();

    QString stype = type().toUpper();

    Releases *releases = Firmware::instance()->f_releases;
    QString relVer = releases->releaseVersion();
    if (ver != relVer) {
        ver = QString("%1->%2").arg(ver).arg(relVer);
    }

    QString s = QString("%1 %2 (%3)").arg(fw).arg(hw).arg(ver);
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

    return releases->loadFirmware(fw, hw, rel_type, &_data, &_offset);
}

void QueueItem::upload()
{
    cleanUploadConnections();
    Firmware::nodes_protocol()->clear_requests();

    if (!loadFirmware(protocol()->hardware(), protocol()->version())) {
        finish(false);
        return;
    }
    ProtocolNodeFile *f = file(type());
    if (!f) {
        finish(false);
        return;
    }
    f->upload(_data, _offset);
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

ProtocolNodeFile *QueueItem::file(const QString &fname)
{
    ProtocolNodeFile *file = protocol()->file(fname);
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

    connect(file_p, &ProtocolNodeFile::uploaded, this, [this]() { finish(true); });
    connect(file_p, &ProtocolNodeFile::error, this, [this]() { finish(false); });
    connect(file_p, &ProtocolNodeFile::interrupted, this, [this]() { finish(false); });

    connect(file_p, &ProtocolNodeFile::valueChanged, this, [this]() { setValue(file_p->value()); });

    return file;
}
