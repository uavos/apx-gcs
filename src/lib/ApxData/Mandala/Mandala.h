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

#include <Protocols/PData.h>

#include "MandalaFact.h"
#include <Fact/Fact.h>
#include <mandala/MandalaBundles.h>
#include <mandala/MandalaMetaTree.h>

class Mandala : public Fact
{
    Q_OBJECT

public:
    explicit Mandala(Fact *parent = nullptr);

    Q_INVOKABLE MandalaFact *fact(mandala::uid_t uid) const;
    Q_INVOKABLE MandalaFact *fact(const QString &mpath, bool silent = false) const;

    QHash<QString, QVariant> constants; // <name,value> constants
    QMap<mandala::uid_t, MandalaFact *> uid_map;

    static const mandala::meta_s &meta(mandala::uid_t uid);
    static mandala::uid_t uid(const QString &mpath);

protected:
    // Fact override
    virtual QString mandalaToString(xbus::pid_raw_t pid_raw) const override;
    virtual xbus::pid_raw_t stringToMandala(const QString &s) const override;

public slots:
    void telemetryData(PBase::Values values, quint64 timestamp_ms);
    void valuesData(PBase::Values values);

signals:
    //forwarded to vehicle
    void sendValue(mandala::uid_t uid, QVariant value);
};
