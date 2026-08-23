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
#include "DatalinkBle.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <serial/CobsDecoder.h>
#include <serial/CobsEncoder.h>

#include <QPermissions>

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdelete-abstract-non-virtual-dtor"
#endif

// Nordic UART Service as implemented by xble nodes
static const QBluetoothUuid nus_svc(QStringLiteral("6e400001-b5a3-f393-e0a9-e50e24dcca9e"));
static const QBluetoothUuid nus_rx(QStringLiteral("6e400002-b5a3-f393-e0a9-e50e24dcca9e"));
static const QBluetoothUuid nus_tx(QStringLiteral("6e400003-b5a3-f393-e0a9-e50e24dcca9e"));

DatalinkBle::DatalinkBle(Fact *parent, QString devName)
    : DatalinkConnection(parent,
                         "ble#",
                         "",
                         "",
                         Datalink::CLIENTS | Datalink::LOCAL,
                         Datalink::CLIENTS | Datalink::LOCAL)
    , m_devName(devName)
{
    setUrl(m_devName);

    // the NUS stream is COBS framed by the node
    setEncoder(new CobsEncoder());
    setDecoder(new CobsDecoder());

    discovery = new QBluetoothDeviceDiscoveryAgent(this);
    connect(discovery,
            &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this,
            &DatalinkBle::deviceDiscovered);
    connect(discovery, &QBluetoothDeviceDiscoveryAgent::finished, this, &DatalinkBle::scanFinished);
    connect(discovery,
            &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this,
            &DatalinkBle::scanError);

    connect(this, &DatalinkConnection::activatedChanged, this, [this]() {
        if (activated()) {
            open();
        } else {
            close();
        }
    });

    scanTimer.setSingleShot(true);
    scanTimer.setInterval(2000);
    connect(&scanTimer, &QTimer::timeout, this, &DatalinkBle::open);
}
DatalinkBle::~DatalinkBle()
{
    if (_encoder)
        delete _encoder;
    if (_decoder)
        delete _decoder;
}

void DatalinkBle::setDevName(QString v)
{
    if (m_devName == v)
        return;
    m_devName = v;

    setUrl(v);

    if (controller || discovery->isActive()) {
        close();
        open();
    }
}

void DatalinkBle::open()
{
    if (!activated())
        return;
    if (controller)
        return;

#if QT_CONFIG(permissions)
    QBluetoothPermission permission;
    permission.setCommunicationModes(QBluetoothPermission::Access);
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, &DatalinkBle::open);
        return;
    case Qt::PermissionStatus::Denied:
        setStatus(tr("Denied"));
        apxMsgW() << tr("Bluetooth permission denied");
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif

    setStatus(tr("Searching"));
    if (!discovery->isActive())
        discovery->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
void DatalinkBle::close()
{
    scanTimer.stop();
    if (discovery->isActive())
        discovery->stop();
    if (service) {
        apxMsg() << tr("BLE disconnected").append(":") << controller->remoteName();
    }
    teardown(false);
    setStatus("");
}

void DatalinkBle::deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (controller)
        return;
    if (!activated())
        return;

    const auto name = info.name();
    if (name.isEmpty())
        return;
    if (m_devName.isEmpty() || m_devName == "auto") {
        if (!name.startsWith("APX-"))
            return;
    } else if (name != m_devName)
        return;

    apxMsg() << tr("BLE device found: %1 %2dBm").arg(name).arg(info.rssi());
    discovery->stop();
    connectToDevice(info);
}
void DatalinkBle::scanFinished()
{
    // scan timed out without a match - rescan while activated
    if (controller)
        return;
    if (!activated())
        return;
    scanTimer.start();
}
void DatalinkBle::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    apxConsoleW() << "BLE scan error:" << error;
    if (controller)
        return;
    if (!activated())
        return;
    setStatus(tr("Unavailable"));
    scanTimer.start();
}

void DatalinkBle::connectToDevice(const QBluetoothDeviceInfo &info)
{
    setStatus(tr("Connecting"));

    controller = QLowEnergyController::createCentral(info, this);

    connect(controller, &QLowEnergyController::connected, this, [this]() {
        controller->discoverServices();
    });
    connect(controller, &QLowEnergyController::discoveryFinished, this, &DatalinkBle::setupService);
    connect(controller, &QLowEnergyController::disconnected, this, [this]() {
        if (service)
            apxMsgW() << tr("BLE disconnected").append(":") << controller->remoteName();
        teardown(true);
    });
    connect(controller,
            &QLowEnergyController::errorOccurred,
            this,
            [this](QLowEnergyController::Error error) {
                apxConsoleW() << "BLE error:" << error;
                teardown(true);
            });

    controller->connectToDevice();
}

void DatalinkBle::setupService()
{
    if (!controller)
        return;

    if (!controller->services().contains(nus_svc)) {
        apxMsgW() << tr("BLE service not found").append(":") << controller->remoteName();
        teardown(true);
        return;
    }
    service = controller->createServiceObject(nus_svc, this);
    if (!service) {
        teardown(true);
        return;
    }
    connect(service, &QLowEnergyService::stateChanged, this, &DatalinkBle::serviceStateChanged);
    connect(service,
            &QLowEnergyService::characteristicChanged,
            this,
            &DatalinkBle::characteristicChanged);
    connect(service,
            &QLowEnergyService::errorOccurred,
            this,
            [](QLowEnergyService::ServiceError error) {
                apxConsoleW() << "BLE service error:" << error;
            });
    service->discoverDetails();
}

void DatalinkBle::serviceStateChanged(QLowEnergyService::ServiceState state)
{
    if (state != QLowEnergyService::RemoteServiceDiscovered)
        return;
    if (!(controller && service))
        return;

    charRx = service->characteristic(nus_rx);
    charTx = service->characteristic(nus_tx);
    if (!(charRx.isValid() && charTx.isValid())) {
        apxMsgW() << tr("BLE characteristics not found").append(":") << controller->remoteName();
        teardown(true);
        return;
    }

    writeMode = (charRx.properties() & QLowEnergyCharacteristic::WriteNoResponse)
                    ? QLowEnergyService::WriteWithoutResponse
                    : QLowEnergyService::WriteWithResponse;

    // subscribe to node -> GCS notifications
    auto cccd = charTx.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (cccd.isValid())
        service->writeDescriptor(cccd, QByteArray::fromHex("0100"));

    setUrl(QString("ble://?name=%1").arg(controller->remoteName()));
    setStatus(tr("Connected"));
    apxMsg() << tr("BLE device connected: %1 mtu %2")
                    .arg(controller->remoteName())
                    .arg(controller->mtu());
    opened();
}

void DatalinkBle::teardown(bool retry)
{
    if (service) {
        service->disconnect(this);
        service->deleteLater();
        service = nullptr;
    }
    charRx = QLowEnergyCharacteristic();
    charTx = QLowEnergyCharacteristic();
    if (controller) {
        controller->disconnect(this);
        if (controller->state() != QLowEnergyController::UnconnectedState)
            controller->disconnectFromDevice();
        controller->deleteLater();
        controller = nullptr;
    }
    closed();
    setUrl(m_devName);
    if (retry && activated()) {
        setStatus(tr("Searching"));
        scanTimer.start();
    }
}

void DatalinkBle::characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() != nus_tx)
        return;
    _rxbuf.append(value);
    readDataAvailable();
}

QByteArray DatalinkBle::read()
{
    if (!service) {
        resetDataStream();
        return {};
    }
    QByteArray data;
    data.swap(_rxbuf);
    return data;
}

void DatalinkBle::write(const QByteArray &packet)
{
    if (!(controller && service && charRx.isValid()))
        return;
    if (service->state() != QLowEnergyService::RemoteServiceDiscovered)
        return;

    // chunk to ATT payload size
    const auto chunk = qMax(20, controller->mtu() - 3);
    for (qsizetype i = 0; i < packet.size(); i += chunk)
        service->writeCharacteristic(charRx, packet.mid(i, chunk), writeMode);
}

void DatalinkBle::resetDataStream()
{
    DatalinkConnection::resetDataStream();
    _rxbuf.clear();
}

DatalinkBleScanner *DatalinkBleScanner::instance()
{
    static auto inst = new DatalinkBleScanner(QCoreApplication::instance());
    return inst;
}
DatalinkBleScanner::DatalinkBleScanner(QObject *parent)
    : QObject(parent)
{
    discovery = new QBluetoothDeviceDiscoveryAgent(this);
    discovery->setLowEnergyDiscoveryTimeout(15000);
    connect(discovery,
            &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this,
            [this](const QBluetoothDeviceInfo &info) {
                const auto name = info.name();
                if (!name.startsWith("APX-"))
                    return;
                if (m_names.contains(name))
                    return;
                m_names.append(name);
                m_names.sort();
                emit namesChanged();
            });
}
void DatalinkBleScanner::scan()
{
    if (discovery->isActive())
        return;

#if QT_CONFIG(permissions)
    QBluetoothPermission permission;
    permission.setCommunicationModes(QBluetoothPermission::Access);
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, &DatalinkBleScanner::scan);
        return;
    case Qt::PermissionStatus::Denied:
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif

    discovery->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
