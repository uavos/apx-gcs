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

#include "PApxNodeFile.h"
#include "PApxVehicle.h"

#include <xbus/XbusNode.h>

class PApxVehicle;

class PApxMission : public PMission
{
    Q_OBJECT

public:
    explicit PApxMission(PApxVehicle *parent);

private:
    PApxVehicle *_vehicle;

    static constexpr const auto file_name{"mission"};

    static QVariantMap _unpack(PStreamReader &stream);
    static QByteArray _pack(const QVariantMap &m);

    PApxNodeFile *_file() const;

    void requestMission() override;
    void updateMission(QVariant var) override;

private slots:
    void parseMissionData(PApxNode *_node,
                          const xbus::node::file::info_s &info,
                          const QByteArray data);

    void updateFiles();
};
