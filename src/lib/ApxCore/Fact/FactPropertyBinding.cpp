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
#include "FactPropertyBinding.h"
#include "Fact.h"

FactPropertyBinding::FactPropertyBinding(Fact *parent, Fact *src, const QString &name)
    : QObject(parent)
    , _src(src)
    , _dst(parent)
    , _name(name)
{
    int pidx_dst = _dst->metaObject()->indexOfProperty(name.toLatin1());
    if (pidx_dst < 0) {
        qWarning() << "misisng property:" << _dst->path() << name;
        return;
    }
    _pdst = _dst->metaObject()->property(pidx_dst);

    int pidx_src = _src->metaObject()->indexOfProperty(name.toLatin1());
    if (pidx_src < 0) {
        qWarning() << "misisng property:" << _src->path() << name;
        return;
    }
    _psrc = _src->metaObject()->property(pidx_src);

    //qDebug() << fact_src->path() << "->" << fact_dst->path() << name;
    if (!_pdst.isWritable()) {
        qWarning() << "readonly property:" << _dst->path() << name;
        connect(_src,
                _psrc.notifySignal().methodSignature().prepend('2'),
                _dst,
                _pdst.notifySignal().methodSignature().prepend('2'));
        return;
    }

    connect(_src,
            _psrc.notifySignal().methodSignature().prepend('2'),
            this,
            SLOT(propertyChanged()));

    connect(_src, &QObject::destroyed, this, [this]() { _dst->unbindProperties(_src); });

    propertyChanged();
}

void FactPropertyBinding::propertyChanged()
{
    //qDebug() << _src->path() << "->" << _dst->path() << _name;
    _pdst.write(_dst, _psrc.read(_src));
}

bool FactPropertyBinding::match(Fact *src, const QString &name)
{
    if (src != nullptr && src != _src)
        return false;
    if (!name.isEmpty() && name != _name)
        return false;
    return true;
}
