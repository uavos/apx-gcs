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
#include "PingRX.h"

PingRX::PingRX(Fact *parent,
               const QString &name,
               const QString &title,
               const QString &descr,
               const QString &icon)
    : Fact(parent, name, title, descr, Group, icon)
{
    f_enabled = new Fact(this, "enable", tr("Enable"), tr("Enable data"), Fact::Bool, "link");

    f_vcpid = new Fact(this,
                       "vcpid",
                       tr("VCP"),
                       tr("Virtual Port Number (ID)"),
                       Fact::Int | Fact::PersistentValue,
                       "numeric");
    f_vcpid->setMin(0);
    f_vcpid->setMax(255);

    _at = new AircraftTraffic(this);

    connect(Fleet::instance(),
            &Fleet::currentChanged,
            this,
            &PingRX::onCurrentUnitChanged,
            Qt::QueuedConnection);
    onCurrentUnitChanged();

    connect(this, &Fact::activeChanged, this, &PingRX::updateStatus);

    App::setContextProperty("QAT", _at);
    qml = loadQml("qrc:/FlightradarPlugin.qml");

#ifdef PINGRX_SIMULATION
    create_test_uav();
    connect(&m_testTimer, &QTimer::timeout, this, &PingRX::update_test_uav);
    m_testTimer.start(200);
#endif // PINGRX_SIMULATION
}

void PingRX::onCurrentUnitChanged()
{
    if (_pdata) {
        disconnect(_pdata, &PData::serialData, this, &PingRX::onPdataSerialData);
    }
    auto protocol = Fleet::instance()->current()->protocol();
    if (protocol)
        _pdata = protocol->data();
    if (_pdata) {
        connect(_pdata, &PData::serialData, this, &PingRX::onPdataSerialData);
    }
}

void PingRX::onPdataSerialData(quint8 portID, QByteArray data)
{
    if (!f_enabled->value().toBool())
        return;

    if (data.isEmpty())
        return;

    if (portID != f_vcpid->value().toInt())
        return;

    if (data.size() != sizeof(GCS_TRAFFIC_REPORT_S))
        return;

    GCS_TRAFFIC_REPORT_S report;

    memcpy(&report, data.constData(), sizeof(GCS_TRAFFIC_REPORT_S));

    _at->updateFromAP(report);
}

void PingRX::updateStatus()
{
    if (active()) {
        setText(QString("VCP#%1").arg(f_vcpid->value().toInt()));
    } else {
        if (!f_enabled->value().toBool()) {
            setText("");
        } else {
            setText("Error");
        }
    }
}
