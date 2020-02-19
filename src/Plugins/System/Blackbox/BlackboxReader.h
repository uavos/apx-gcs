﻿/*
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
#ifndef BlackboxReader_H
#define BlackboxReader_H
//=============================================================================
#include <Fact/Fact.h>
#include <Xbus/uart/EscDecoder.h>
class Vehicle;
class ProtocolVehicle;
//=============================================================================
class BlackboxReader : public Fact
{
    Q_OBJECT

public:
    explicit BlackboxReader(Fact *parent, QString callsign, QString uid);

    void processData(QByteArray data);

private:
    ProtocolVehicle *protocol;
    Vehicle *vehicle;
    quint32 dataCnt;

    EscDecoder<1024 * 8> esc_reader;
};
//=============================================================================
#endif
