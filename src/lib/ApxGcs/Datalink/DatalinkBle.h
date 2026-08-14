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

#include "DatalinkConnection.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QtCore>

class DatalinkBle : public DatalinkConnection
{
    Q_OBJECT

public:
    explicit DatalinkBle(Fact *parent, QString devName);
    ~DatalinkBle() override;

    void setDevName(QString v);

private:
    QString m_devName;

    QBluetoothDeviceDiscoveryAgent *discovery;
    QLowEnergyController *controller{};
    QLowEnergyService *service{};
    QLowEnergyCharacteristic charRx; // GCS -> node (write)
    QLowEnergyCharacteristic charTx; // node -> GCS (notify)
    QLowEnergyService::WriteMode writeMode{QLowEnergyService::WriteWithoutResponse};

    QByteArray _rxbuf;
    QTimer scanTimer;

    void connectToDevice(const QBluetoothDeviceInfo &info);
    void teardown(bool retry);

protected:
    //DatalinkConnection overrided
    void open() override;
    void close() override;
    QByteArray read() override;
    void write(const QByteArray &packet) override;
    void resetDataStream() override;

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void scanFinished();
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void setupService();
    void serviceStateChanged(QLowEnergyService::ServiceState state);
    void characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
};

// collects APX-* device names for the port config url dropdown
class DatalinkBleScanner : public QObject
{
    Q_OBJECT

public:
    static DatalinkBleScanner *instance();

    QStringList names() const { return m_names; }
    void scan();

signals:
    void namesChanged();

private:
    explicit DatalinkBleScanner(QObject *parent);

    QBluetoothDeviceDiscoveryAgent *discovery;
    QStringList m_names;
};
