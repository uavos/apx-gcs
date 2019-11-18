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
//=============================================================================
QueueItem::QueueItem(Fact *parent,
                     const QString &nodeName,
                     const QString &nodeDescr,
                     const QString &sn,
                     const QString &hw,
                     const QString &ver,
                     Firmware::UpgradeType type)
    : Fact(parent, QString("fw_%1").arg(sn), "", "")
    , nodeName(nodeName)
    , nodeDescr(nodeDescr)
    , sn(sn)
    , hw(hw)
    , ver(ver)
    , type(type)
{
    if (type == Firmware::LD)
        setIcon("alert-circle");
    else
        setIcon("chip");

    updateDescr();
}
//=============================================================================
QueueItem::QueueItem(Fact *parent, const QString &name)
    : Fact(parent, name, "", "")
    , type(Firmware::Any)
{}
//=============================================================================
void QueueItem::updateDescr()
{
    QString s = nodeName.toUpper();
    if (!nodeDescr.isEmpty())
        s.append(QString(" (%2)").arg(nodeDescr));
    setTitle(s);
    QStringList st;
    st << QMetaEnum::fromType<Firmware::UpgradeType>().valueToKey(type);
    st << hw;
    st << ver;
    setDescr(st.join(' '));
}
//=============================================================================
