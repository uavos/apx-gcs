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
#include "DatalinkStats.h"
#include "Datalink.h"
//=============================================================================
DatalinkStats::DatalinkStats(Datalink *parent)
    : Fact(parent,
           "stats",
           tr("Statistics"),
           tr("Data traffic usage"),
           Group | FlatModel,
           "chart-line")
{
    f_datalink = parent;

    f_uplink = new DatalinkStatsCounter(this, "uplink", tr("Uplink"), "");
    f_dnlink = new DatalinkStatsCounter(this, "dnlink", tr("Downlink"), "");
    f_total = new DatalinkStatsCounter(this, "total", tr("Total"), "");

    connect(f_datalink, &Datalink::packetReceived, f_dnlink, [this](QByteArray packet) {
        uint sz = static_cast<uint>(packet.size());
        f_dnlink->countData(sz);
        f_total->countData(sz);
    });
    connect(f_datalink, &Datalink::packetTransmitted, f_dnlink, [this](QByteArray packet) {
        uint sz = static_cast<uint>(packet.size());
        f_uplink->countData(sz);
        f_total->countData(sz);
    });
}
//=============================================================================
//=============================================================================
DatalinkStatsCounter::DatalinkStatsCounter(DatalinkStats *parent,
                                           QString name,
                                           QString title,
                                           QString descr)
    : Fact(parent, name, title, descr, Section)
    , packetCnt(0)
    , packetCntT(0)
    , packetRate(0)
    , dataCnt(0)
    , dataCntT(0)
    , dataRate(0)
{
    f_cnt = new Fact(this, "cnt", tr("Packets counter"), "");
    f_cnt->setSection(title);
    f_cnt->setValue(0);
    f_rate = new Fact(this, "rate", tr("Packets rate"), "");
    f_rate->setSection(title);
    f_rate->setValue(0);
    f_datacnt = new Fact(this, "datacnt", tr("Data counter"), "");
    f_datacnt->setSection(title);
    f_datacnt->setValue(0);
    f_datarate = new Fact(this, "datarate", tr("Data rate"), "");
    f_datarate->setSection(title);
    f_datarate->setValue(0);

    updateTimer.setSingleShot(false);
    updateTimer.setInterval(1777);
    connect(&updateTimer, &QTimer::timeout, this, &DatalinkStatsCounter::updateTimerTimeout);
    updateTimer.start();

    time.start();
}
//=============================================================================
void DatalinkStatsCounter::countData(uint size)
{
    packetCnt++;
    dataCnt += size;
    f_cnt->setValue(packetCnt);
}
//=============================================================================
void DatalinkStatsCounter::updateTimerTimeout()
{
    //data count
    f_datacnt->setValue(dataToString(dataCnt));

    //rate
    int t = time.restart();
    f_datarate->setValue(dataToString(getRate(&dataRate, dataCnt - dataCntT, t)) + "/sec");
    dataCntT = dataCnt;
    f_rate->setValue(QString::number((packetCnt - packetCntT) * 1000 / t) + "/sec");
    packetCntT = packetCnt;
}
//=============================================================================
double DatalinkStatsCounter::getRate(double *v, uint dcnt, uint t)
{
    double vt = (double) dcnt / (double) t * 1000.0;
    *v = (*v) * 0.5 + vt * 0.5;
    return *v;
}
//=============================================================================
QString DatalinkStatsCounter::dataToString(uint v)
{
    if (v < 1024)
        return QString("%1 bytes").arg(v);
    else if (v < (1024 * 1024))
        return QString("%1 KB").arg(v / 1024.0, 0, 'f', 1);
    else if (v < (1024 * 1024 * 1024))
        return QString("%1 MB").arg(v / (1024.0 * 1024.0), 0, 'f', 2);
    else
        return QString("%1 GB").arg(v / (1024.0 * 1024.0 * 1024.0), 0, 'f', 3);
}
//=============================================================================
