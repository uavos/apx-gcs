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
#ifndef Initialize_H
#define Initialize_H
//=============================================================================
#include "Firmware.h"
#include <QSerialPort>
//=============================================================================
class Initialize : public Fact
{
    Q_OBJECT

public:
    explicit Initialize(Firmware *firmware, Fact *parent);

    Fact *f_node;
    Fact *f_hw;
    Fact *f_fw;
    Fact *f_port;
    Fact *f_continuous;

    FactAction *f_start;

private:
    Firmware *firmware;
    QSerialPort *dev;

private slots:
    void updateNodeEnums();
    void updateHwEnums();
    void updatePortEnums();

    void startTriggered();
};
//=============================================================================
#endif
