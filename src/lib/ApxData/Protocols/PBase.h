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

#include "PTrace.h"
#include "PTreeBase.h"

class PVehicle;
class PFirmware;
class PTrace;

class PBase : public PTreeBase
{
    Q_OBJECT

public:
    explicit PBase(Fact *parent, QString name, QString title, QString descr);

    virtual PTrace *trace() override { return _trace; }

    virtual void send_uplink(QByteArray packet) override;

    virtual void process_downlink(QByteArray packet) = 0;

    typedef QHash<mandala::uid_t, QVariant> Values;

    // interface to node firmware loader
    PFirmware *firmware() const { return m_firmware; }

private:
    PTrace *_trace;

protected:
    PFirmware *m_firmware{};

    // data comm
public slots:
    void rx_data(QByteArray packet); //connect rx data stream
signals:
    void tx_data(QByteArray packet); //connect tx interface

    // signaled when a new vehicle is identified to provide interface
    void vehicle_available(PVehicle *vehicle);
};
