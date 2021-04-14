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
#pragma once

#include <QtCore>

#include "PTrace.h"

#include <Fact/Fact.h>

#include <mandala/MandalaMetaTree.h>

class PBase;

class PTreeBase : public Fact
{
    Q_OBJECT

public:
    explicit PTreeBase(Fact *parent,
                       QString name,
                       QString title,
                       QString descr = QString(),
                       Flags flags = Flags(NoFlags));

    template<typename T = PTreeBase>
    T *parent() const
    {
        return qobject_cast<T *>(parentFact());
    }

    auto root() const { return findParent<PBase>(); }

    virtual void send_uplink(QByteArray packet);

    virtual PTrace *trace();

protected:
    void _nimp(QString fname);
};
