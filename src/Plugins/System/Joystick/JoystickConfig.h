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

// Editor of a saved user configuration.
// The configuration is edited without the device: the device is described
// by its name and by the numbers of controls, which are set by the user.
class JoystickConfig : public Fact
{
    Q_OBJECT
public:
    explicit JoystickConfig(Fact *parent);

    Fact *f_title;
    Fact *f_name;

    // numbers of controls to show, not saved
    Fact *f_naxes;
    Fact *f_nbuttons;
    Fact *f_nhats;

    Fact *f_axes;
    Fact *f_buttons;
    Fact *f_hats;

    Fact *f_save;
    Fact *f_remove;

    // the configuration as it is saved, identifies the record to update
    QJsonObject stored;

    void load(const QJsonObject &config);
    QJsonObject save() const;

    // edited settings are shown as modified until they are saved
    void markSaved();

private:
    // number of controls up to the last one which is set
    static int used(Fact *controls);

private slots:
    void updateControls();

signals:
    void changed(); // save of the edited settings is requested
};
