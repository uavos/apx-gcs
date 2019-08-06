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
#include "JoystickAxis.h"
#include <ApxApp.h>
#include <ApxLog.h>

//=============================================================================
JoystickAxis::JoystickAxis(Fact *parent)
    : Fact(parent,
           QString("axis%1").arg(parent->size()),
           QString::number(parent->size() + 1),
           "",
           Group | Text)
{
    _value = 0.0;

    f_hyst = new Fact(this, "hyst", tr("Hysterezis"), tr("Filter zero position"), Float);
    f_hyst->setMin(0.0);
    f_hyst->setMax(0.8);
    connect(f_hyst, &Fact::valueChanged, this, [this]() {
        setDescr(QString("%1: %2").arg(f_hyst->name()).arg(f_hyst->text()));
    });
}
//=============================================================================
void JoystickAxis::update(qreal v)
{
    v = round(v * 100) / 100.0;
    if (_value == v)
        return;
    _value = v;
    setStatus(QString::number(v, 'f', 2).append(" > "));
    QString s = text().simplified();
    if (s.isEmpty())
        return;
    s.replace("$", QString("(%1)").arg(v));
    ApxApp::jsexec(s);
}
//=============================================================================
void JoystickAxis::loadConfig(const QJsonObject &config)
{
    setValue(config["scr"].toString());
    f_hyst->setValue(config["hyst"].toDouble());
}
QJsonObject JoystickAxis::saveConfig()
{
    QJsonObject v;
    v.insert("id", num() + 1);
    v.insert("scr", text().simplified());
    v.insert("hyst", f_hyst->value().toDouble());
    return v;
}
//=============================================================================
