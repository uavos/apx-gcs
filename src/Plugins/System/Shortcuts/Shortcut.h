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
#ifndef Shortcut_H
#define Shortcut_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
class Shortcuts;
//=============================================================================
class Shortcut : public Fact
{
    Q_OBJECT

public:
    explicit Shortcut(Fact *parent, Shortcuts *shortcuts, const Shortcut *sc, bool bUsr);

    Fact *_enabled;
    Fact *_key;
    Fact *_cmd;

    Fact *_save;
    Fact *_remove;

    QJsonObject valuesToJson(bool array = false) const override;
    void valuesFromJson(const QJsonObject &jso) override;

private:
    Shortcuts *container;
    bool _new;
    bool bUsr;

private slots:
    void updateStats();
    void enable();
    void disable();

public slots:
    void defaults();
};
//=============================================================================
#endif
