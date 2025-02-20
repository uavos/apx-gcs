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
#include "PApxRequest.h"
#include "PApx.h"

PApxRequest::PApxRequest(PTreeBase *parent)
    : PStreamWriter(_txbuf, sizeof(_txbuf))
    , _parent(parent)
{}

void PApxRequest::request(mandala::uid_t uid, bool req)
{
    pid.uid = uid;
    pid.eid = xbus::eid_none;
    pid.req = req;

    request(pid);
}

void PApxRequest::request(const xbus::pid_s &pid)
{
    reset();
    this->pid = pid;
    pid.write(this);

    PTrace *t = _parent->trace();
    if (t->enabled()) {
        t->uplink();
        _parent->findParent<PApx>()->trace_pid(pid);
    }
}

void PApxRequest::send()
{
    _parent->send_uplink(get_packet());
}
QByteArray PApxRequest::get_packet()
{
    pid.seq++;
    return toByteArray();
}
