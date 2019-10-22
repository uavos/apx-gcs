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
#ifndef Shortcuts_H
#define Shortcuts_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
class Shortcut;
class AppSettings;
//=============================================================================
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
//=============================================================================
#endif
