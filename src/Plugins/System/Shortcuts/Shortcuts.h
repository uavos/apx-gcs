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
class Shortcut;
class AppSettings;

class Shortcuts : public Fact
{
    Q_OBJECT
public:
    explicit Shortcuts(Fact *parent);

    Q_INVOKABLE QString keyToPortableString(int key, int modifier) const;

    Fact *f_blocked;

    Fact *f_allonSys;
    Fact *f_alloffSys;
    Fact *f_allonUsr;
    Fact *f_alloffUsr;

    Fact *f_usr;
    Fact *f_sys;

    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

private:
    Shortcut *f_add;

    QTimer saveTimer;

    void addUserShortcut();

private slots:
    void saveDo();

public slots:
    void updateStats();
    void addTriggered();

    void load();
    void save();
};
