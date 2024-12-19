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

#include "PBase.h"
#include "PData.h"
#include "PFirmware.h"
#include "PNode.h"
#include "PNodes.h"
#include "PTelemetry.h"

#include "PUnit.h"

class PBase;
class PUnit;

class Protocols : public Fact
{
    Q_OBJECT
public:
    explicit Protocols(Fact *parent);

    void setTraceEnabled(bool v);

    auto current() const { return _protocol; }

private:
    Fact *f_current;

    PBase *_protocol{};

    void updateNames();

private slots:
    void updateProtocol();

    // data comm
public slots:
    void rx_data(QByteArray packet); //connect rx data stream
signals:
    void tx_data(QByteArray packet); //connect tx interface

    // interface provider
    void unit_available(PUnit *unit);
    void trace_packet(QStringList blocks);
};
