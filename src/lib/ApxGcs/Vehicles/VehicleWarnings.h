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
#ifndef VehicleWarnings_H
#define VehicleWarnings_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
class Vehicle;
//=============================================================================
class VehicleWarnings : public Fact
{
    Q_OBJECT
    Q_ENUMS(MsgType)

public:
    explicit VehicleWarnings(Vehicle *parent);

    enum MsgType { INFO = 0, WARNING, ERROR };
    Q_ENUM(MsgType)

    Fact *f_clear;

private:
    QTimer showTimer;
    Fact *createItem(const QString &msg, MsgType kind);

    QHash<Fact *, int> showMap;
    QList<Fact *> showList;
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
//=============================================================================
#endif
