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
#include <QVariant>
#include <QtCore>

class FactValueBase : public QObject
{
    Q_OBJECT
public:
    FactValueBase(Fact *fact = 0)
        : QObject(fact)
        , f(nullptr)
    {
        setFact(fact);
    }
    void setFact(Fact *v)
    {
        if (f == v)
            return;
        if (f)
            disconnect(f, &Fact::valueChanged, this, &FactValueBase::valueChanged);
        f = v;
        connect(f, &Fact::valueChanged, this, &FactValueBase::valueChanged);
        emit valueChanged();
    }
    Fact *fact() { return f; }

protected:
    QPointer<Fact> f;
signals:
    void valueChanged();
};

template<typename T>
class FactValue : public FactValueBase
{
public:
    FactValue(Fact *fact = 0)
        : FactValueBase(fact)
    {}
    T &operator=(const T &v)
    {
        f->setValue(v);
        return f->value();
    }
    operator T() const { return qvariant_cast<T>(f->value()); }
};
