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
class Unit;

class UnitWarnings : public Fact
{
    Q_OBJECT
    Q_ENUMS(MsgType)

public:
    explicit UnitWarnings(Unit *parent);

    enum MsgType { INFO = 0, WARNING, ERROR };
    Q_ENUM(MsgType)

    Fact *f_clear;

private:
    QTimer showTimer;
    Fact *createItem(const QString &msg, MsgType kind);

    QHash<Fact *, int> showMap;
    FactList showList;
    int showNum;
private slots:
    void showTimerTimeout();
public slots:
    void warning(const QString &msg);
    void error(const QString &msg);
signals:
    void show(QString msg, MsgType msgType);
    void showMore(QString msg, MsgType msgType);
};
