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

#include "PBase.h"

class PUnit;

class PData : public PTreeBase
{
    Q_OBJECT

public:
    explicit PData(PUnit *parent);

protected:
    PUnit *_unit;

private slots:
    void updateStreamType();

public slots:
    virtual void sendValue(mandala::uid_t uid, QVariant value) { _nimp(__FUNCTION__); }

    virtual void requestCalibration(mandala::uid_t uid, QByteArray data) { _nimp(__FUNCTION__); }
    virtual void requestScript(QString func, QVariant arg = {}) { _nimp(__FUNCTION__); }
    virtual void sendSerial(quint8 portID, QByteArray data) { _nimp(__FUNCTION__); }

signals:
    void valuesData(PBase::Values values);

    void calibrationData(mandala::uid_t uid, QByteArray data);
    void serialData(quint8 portID, QByteArray data);
    void jsexecData(QString script);
};
