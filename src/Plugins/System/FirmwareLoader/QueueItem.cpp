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

#include <App/AppGcs.h>

QueueItem::QueueItem(Fact *parent, ProtocolNode *protocol)
    : Fact(parent, QString("fw_%1").arg(protocol->sn()), "", "")
{
    m_protocol = new ProtocolNode(AppGcs::instance()->protocol->local->nodes, protocol->sn());
    m_protocol->setParent(this);

    /*if (type == Firmware::LD)
        setIcon("alert-circle");
    else
        setIcon("chip");*/

    updateDescr();
}

bool QueueItem::match(const QString &sn) const
{
    return m_protocol->sn() == sn;
}
QueueItem::UpgradeType QueueItem::type() const
{
    return m_type;
}

void QueueItem::updateDescr()
{
    /*QString s = nodeName.toUpper();
    if (!nodeDescr.isEmpty())
        s.append(QString(" (%2)").arg(nodeDescr));
    setTitle(s);
    QStringList st;
    st << QMetaEnum::fromType<Firmware::UpgradeType>().valueToKey(type);
    st << hw;
    st << ver;
    setDescr(st.join(' '));*/
}
