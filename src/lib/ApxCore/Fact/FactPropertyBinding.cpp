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
#include "FactPropertyBinding.h"
#include "Fact.h"

FactPropertyBinding::FactPropertyBinding(Fact *parent,
                                         Fact *src,
                                         const QString &name,
                                         FactPropertyBinding *src_binding)
    : QObject(parent)
    , _src(src)
    , _dst(parent)
    , _name(name)
    , _src_binding(src_binding)
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
        _clist.append(connect(_src,
                              _psrc.notifySignal().methodSignature().prepend('2'),
                              _dst,
                              _pdst.notifySignal().methodSignature().prepend('2')));
        return;
    }

    connect(_src,
            _psrc.notifySignal().methodSignature().prepend('2'),
            this,
            SLOT(propertyChanged()));

    connect(_src, &QObject::destroyed, this, [this]() { _dst->unbindProperties(_src); });

    if (_src_binding)
        _src_binding->_src_binding = this;

    propertyChanged();
}
FactPropertyBinding::~FactPropertyBinding()
{
    for (auto c : _clist)
        disconnect(c);
}

void FactPropertyBinding::propertyChanged()
{
    //qDebug() << _src->name() << "->" << _dst->name() << _name;
    if (_blocked) {
        //qDebug() << "blocked" << _src->name() << "->" << _dst->name() << _name;
        return;
    }
    QVariant v = _psrc.read(_src);
    if (v == _pdst.read(_dst))
        return;

    if (_src_binding)
        _src_binding->block();

    _pdst.write(_dst, v);

    if (_src_binding)
        _src_binding->unblock();
}

bool FactPropertyBinding::match(Fact *src, const QString &name)
{
    if (src != nullptr && src != _src)
        return false;
    if (!name.isEmpty() && name != _name)
        return false;
    return true;
}

void FactPropertyBinding::block()
{
    _blocked = true;
}
void FactPropertyBinding::unblock()
{
    _blocked = false;
}
