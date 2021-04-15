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
#include "PApxFirmware.h"

#include "PApxNode.h"
#include "PApxNodes.h"

#include <App/App.h>

PApxFirmware::PApxFirmware(PApx *parent)
    : PFirmware(parent)
    , _nodes(static_cast<PApxNodes *>(parent->local()->nodes()))
{}

void PApxFirmware::upgradeFirmware(QString uid, QString name, QByteArray data, quint32 offset)
{
    qDebug() << uid << name << QString::number(offset, 16);
    PFirmware::upgradeFirmware(uid, name, data, offset);

    _retry = 0;
    start();
}

void PApxFirmware::start()
{
    if (_uid.isEmpty())
        return;

    _node = _nodes->getNode(_uid);
    emit upgradeStarted(_uid, _name);

    qDebug() << _name << _uid;

    if (_name == "ldr") {
        rebootFinished();
    } else {
        auto req = new PApxNodeRequestReboot(_node, xbus::node::reboot::loader);
        req->silent = true;
        connect(req, &PApxNodeRequest::finished, this, &PApxFirmware::rebootFinished);
    }
}

void PApxFirmware::rebootFinished()
{
    if (!_node)
        return;

    qDebug() << _name << _uid;

    auto req = new PApxNodeRequestIdent(_node);
    req->silent = true;
    connect(req, &PApxNodeRequest::finished, this, &PApxFirmware::identFinished);
}
void PApxFirmware::identFinished()
{
    qDebug() << _name << _uid << _node;

    if (!_node)
        return;

    if (_node->file(_name)) {
        auto req = new PApxNodeRequestFileWrite(_node, _name, _data, _offset);
        connect(req, &PApxNodeRequest::finished, this, &PApxFirmware::finish);
        connect(req, &PApxNodeRequestFile::uploaded, this, &PApxFirmware::fileUploaded);
        connect(req, &PApxNodeRequestFile::progress, this, &PApxFirmware::fileProgress);
    } else if (++_retry <= 5) {
        apxMsg() << tr("Upgrade retry %1/%2...").arg(_retry).arg(5);
        start();
    } else {
        finish();
    }
}

void PApxFirmware::fileUploaded()
{
    if (!_node)
        return;

    qDebug() << _name << _uid;

    _success = true;
}

void PApxFirmware::fileProgress(int percent)
{
    setProgress(percent);
    int tcnt = percent * _data.size() / 100;
    setValue(QString("%1/%2")
                 .arg(AppRoot::capacityToString(tcnt, 1))
                 .arg(AppRoot::capacityToString(_data.size())));
}

void PApxFirmware::finish()
{
    if (!_node)
        return;

    _node = {};
    _retry = 0;

    PFirmware::finish();
}
