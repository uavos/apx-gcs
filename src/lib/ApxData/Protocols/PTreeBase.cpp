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
#include "PTreeBase.h"
#include "PBase.h"

#include <App/AppLog.h>

PTreeBase::PTreeBase(Fact *parent, QString name, QString title, QString descr, Flags flags)
    : Fact(parent, name, title, descr, flags)
{}

void PTreeBase::send_uplink(QByteArray packet)
{
    if (parent()) {
        parent()->send_uplink(packet);
    }
}

void PTreeBase::_nimp(QString fname)
{
    apxMsgW() << tr("Not implemented").append(':') << fname;
}

PTrace *PTreeBase::trace()
{
    // TODO find a better way for trace function (I still don't like the implementation though)
    return parent() ? parent()->trace() : new PTrace(this);
}
