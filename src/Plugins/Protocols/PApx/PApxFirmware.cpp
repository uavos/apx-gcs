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

PApxFirmware::PApxFirmware(PApx *parent)
    : PFirmware(parent)
    , _nodes(static_cast<PApxNodes *>(parent->local()->nodes()))
{}

void PApxFirmware::upgradeFirmware(QString uid, QString name, QByteArray data, quint32 offset)
{
    qDebug() << uid << name << QString::number(offset, 16);
    PFirmware::upgradeFirmware(uid, name, data, offset);

    _success = false;

    _uid = uid;
    _name = name;
    _data = data;
    _offset = offset;

    _node = _nodes->getNode(uid);
    connect(_node, &PNode::identReceived, this, &PApxFirmware::identReceived, Qt::UniqueConnection);
    _node->requestIdent();
}

void PApxFirmware::identReceived(QVariantMap ident)
{
    if (!_node || _node->uid() != _uid)
        return;

    if (_node->file(_name)) {
        auto req = new PApxNodeRequestFileWrite(_node, _name, _data, _offset);
        connect(req, &PApxNodeRequest::finished, this, &PApxFirmware::fileFinished);
        connect(req, &PApxNodeRequestFile::uploaded, this, &PApxFirmware::fileUploaded);
    }
}

void PApxFirmware::fileUploaded()
{
    _success = true;
}

void PApxFirmware::fileFinished()
{
    emit upgradeFinished(_uid, _success);
    _success = false;
}
