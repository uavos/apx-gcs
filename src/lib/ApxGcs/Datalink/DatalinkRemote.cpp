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
#include "DatalinkRemote.h"
#include "Datalink.h"
#include "DatalinkRemotes.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <tcp_ports.h>

DatalinkRemote::DatalinkRemote(Fact *parent, Datalink *datalink, QUrl url)
    : DatalinkSocketHttp(parent, url)
    , datalink(datalink)
{
    updateStatsTimer.setSingleShot(true);
    connect(&updateStatsTimer, &QTimer::timeout, this, &DatalinkRemote::updateStats);

    updateStats();
}

void DatalinkRemote::updateStats()
{
    if (time.isValid()) {
        int t = time.elapsed() / 1000;
        setDescr(QString("%1 (%2)")
                     .arg(t >= 60 ? tr("No service") : tr("Alive"))
                     .arg(t == 0    ? tr("now")
                          : t >= 60 ? QString("%1 %2").arg(t / 60).arg(tr("min"))
                                    : QString("%1 %2").arg(t).arg(tr("sec"))));
        updateStatsTimer.start(t > 60 ? 60000 : 5000);
    }
}

void DatalinkRemote::updateTimeout()
{
    time.start();
    updateStatsTimer.start(1000);
}

void DatalinkRemote::open()
{
    //check if already present in connections
    if (datalink->findActiveConnection(_hostAddress)) {
        setActivated(false);
        return;
    }
    //open
    DatalinkSocketHttp::open();
}
