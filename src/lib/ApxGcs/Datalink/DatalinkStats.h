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
#ifndef DatalinkStats_H
#define DatalinkStats_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
class Datalink;
class DatalinkStatsCounter;
//=============================================================================
class DatalinkStats : public Fact
{
    Q_OBJECT
public:
    explicit DatalinkStats(Datalink *parent);

    Datalink *f_datalink;

    DatalinkStatsCounter *f_uplink;
    DatalinkStatsCounter *f_dnlink;

    DatalinkStatsCounter *f_total;
};
//=============================================================================
class DatalinkStatsCounter : public Fact
{
    Q_OBJECT
public:
    explicit DatalinkStatsCounter(DatalinkStats *parent, QString name, QString title, QString descr);

    Fact *f_cnt;
    Fact *f_rate;
    Fact *f_datacnt;
    Fact *f_datarate;

private:
    uint packetCnt;
    uint packetCntT;
    double packetRate;

    uint dataCnt;
    uint dataCntT;
    double dataRate;

    QElapsedTimer time;
    QTimer updateTimer;

    QString dataToString(uint v);
    double getRate(double *v, uint dcnt, uint t);

private slots:
    void updateTimerTimeout();

public slots:
    void countData(uint size);
};
//=============================================================================
#endif
