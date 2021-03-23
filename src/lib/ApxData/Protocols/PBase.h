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

#include <Fact/Fact.h>

#include "PTreeBase.h"
#include "PVehicle.h"
#include "PVehicles.h"

class PVehicles;

class PBase : public PTreeBase
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)

public:
    explicit PBase(Fact *parent, QString name, QString title, QString descr);

    // interface to vehicles root protocol
    PVehicles *vehicles() const { return m_vehicles; }

    virtual PTrace *trace() const override { return _trace; }

    virtual void send_uplink(QByteArray packet) override;
    virtual void process_downlink(QByteArray packet) override;

private:
    PTrace *_trace;

protected:
    PVehicles *m_vehicles{};

    // data comm
public slots:
    void rx_data(QByteArray packet); //connect rx data stream
signals:
    void tx_data(QByteArray packet); //connect tx interface
};
