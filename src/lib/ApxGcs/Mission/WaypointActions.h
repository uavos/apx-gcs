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
class Waypoint;

class WaypointActions : public Fact
{
    Q_OBJECT

public:
    explicit WaypointActions(Waypoint *parent);

    Fact *f_speed;
    Fact *f_poi;
    Fact *f_script;

    QJsonValue toJson() override;

private:
    bool blockActionsValueChanged;

protected:
    void hashData(QCryptographicHash *h) const override;

private slots:
    void updateActionsValue();
    void actionsValueChanged();
};
