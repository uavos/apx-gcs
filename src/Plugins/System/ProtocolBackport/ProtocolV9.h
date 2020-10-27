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

#include <Protocols/ProtocolConverter.h>

#include <xbus/XbusStreamReader.h>
#include <xbus/XbusStreamWriter.h>

class ProtocolV9 : public ProtocolConverter
{
    Q_OBJECT
public:
    explicit ProtocolV9(QObject *parent = nullptr);

private:
    QByteArray out;
    XbusStreamWriter stream;

    QMap<quint16, quint16> downlinkIdMap;
    QMap<quint16, quint16> uplinkIdMap;

    void parseDownlink(XbusStreamReader &is);
    void parseUplink(XbusStreamReader &is);

    void copy(const XbusStreamReader &is);

protected:
    void convertDownlink(const QByteArray &packet) override;
    void convertUplink(const QByteArray &packet) override;

    bool convertDownlinkId(quint16 *id) override;
    bool convertUplinkId(quint16 *id) override;
};
