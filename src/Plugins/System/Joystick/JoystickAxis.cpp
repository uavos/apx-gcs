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
#include "JoystickAxis.h"
#include <App/App.h>
#include <App/AppLog.h>
#include <cmath>

//=============================================================================
JoystickAxis::JoystickAxis(Fact *parent)
    : Fact(parent,
           QString("axis%1").arg(parent->size()),
           QString::number(parent->size() + 1),
           "",
           Group | Text)
{
    _value = 0.0;
    _hyst = 0.0;

    f_hyst = new Fact(this, "hyst", tr("Hysterezis"), tr("Filter zero position"), Float);
    f_hyst->setMin(0.0);
    f_hyst->setMax(0.8);
    connect(f_hyst, &Fact::valueChanged, this, [this]() { _hyst = f_hyst->value().toDouble(); });
    connect(f_hyst, &Fact::valueChanged, this, &JoystickAxis::updateDescr);
}
//=============================================================================
void JoystickAxis::updateDescr()
{
    QStringList st;
    if (f_hyst->value().toDouble() != 0.0)
        st << QString("%1: %2").arg(f_hyst->name()).arg(f_hyst->text());
    if (_value != 0.0)
        st << QString("value: %1").arg(QString::number(_value, 'f', 2));
    setDescr(st.join(' '));
}
//=============================================================================
void JoystickAxis::update(qreal v)
{
    v = round(v * 100) / 100.0;
    if (std::abs(v) < _hyst)
        v = 0;
    else if (v > 0.0)
        v = (v - _hyst) / (1.0 - _hyst);
    else
        v = (v + _hyst) / (1.0 - _hyst);

    if (_value == v)
        return;

    _value = v;
    setActive(v != 0.0);
    updateDescr();
    //setStatusText(QString::number(v, 'f', 2).append(" > "));
    QString s = text().simplified();
    if (s.isEmpty())
        return;
    s.replace("$", QString("(%1)").arg(v));
    App::jsexec(s);
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
