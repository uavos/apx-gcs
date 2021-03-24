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

#include "PApxVehicle.h"

class PApxVehicle;

class PApxData : public PData
{
    Q_OBJECT

public:
    explicit PApxData(PApxVehicle *parent);

    bool process_downlink(const xbus::pid_s &pid, PStreamReader &stream);

private:
    PApxRequest _req;

    template<typename S>
    void sendBundle(mandala::uid_t uid, const S &data)
    {
        _req.request(uid, xbus::pri_final);
        _req.write(&data, sizeof(S));
        trace()->raw(data);
        _req.send();
    }
    static void pack(const QVariant &v, mandala::type_id_e type, PStreamWriter &stream);

    template<typename T>
    static QVariant unpack(PStreamReader &stream)
    {
        if (stream.available() < sizeof(T))
            return QVariant();
        return QVariant::fromValue(stream.read<T>());
    }
    static PBase::Values unpack(const xbus::pid_s &pid,
                                const mandala::spec_s &spec,
                                PStreamReader &stream);

protected:
    void sendValue(mandala::uid_t uid, QVariant value) override;

    void requestCalibration(mandala::uid_t uid, QByteArray data) override;
    void requestScript(QString func) override;
    void sendSerial(quint8 portID, QByteArray data) override;

    void flyTo(qreal lat, qreal lon) override;
};
