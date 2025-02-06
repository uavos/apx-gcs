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

#include "MandalaFact.h"

// mandala value units converter
class MandalaConverter : public QObject
{
    Q_OBJECT

public:
    using lambda_t = std::function<QVariant(const QVariant &v)>;

    explicit MandalaConverter(MandalaFact *fact,
                              QStringList units,
                              lambda_t func_from_orig,
                              lambda_t func_to_orig)
        : QObject(fact)
        , _fact(fact)
        , _units(units)
        , _func_from_orig(func_from_orig)
        , _func_to_orig(func_to_orig)
    {
        fact->addConverter(this);
        if (units.size() == 1) {
            fact->setUnits(units.at(0));
        } else if (units.size() >= 2) {
            fact->setUnits(fact->units().replace(units.at(0), units.at(1)));
        }
    }

    explicit MandalaConverter(MandalaFact *fact, QStringList units, double scale)
        : MandalaConverter(
              fact,
              units,
              [scale](const QVariant &v) { return QVariant::fromValue(v.toDouble() * scale); },
              [scale](const QVariant &v) { return QVariant::fromValue(v.toDouble() / scale); })
    {}

    virtual ~MandalaConverter() { _fact->removeConverter(this); }

    virtual QVariant from_orig(const QVariant &v) const { return _func_from_orig(v); }
    virtual QVariant to_orig(const QVariant &v) const { return _func_to_orig(v); }

private:
    MandalaFact *_fact;
    QStringList _units;
    lambda_t _func_from_orig;
    lambda_t _func_to_orig;
};
