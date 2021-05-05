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

#include <Fact/Fact.h>
#include <QtCore>
class Datalink;
class DatalinkStatsCounter;

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
