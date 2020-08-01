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
#include "ProtocolVehicle.h"

#include <Mandala/Mandala.h>

#include <Xbus/XbusNode.h>
#include <Xbus/XbusPacket.h>

#include <Database/VehiclesReqVehicle.h>

ProtocolVehicle::ProtocolVehicle(ProtocolVehicles *vehicles,
                                 xbus::vehicle::squawk_t squawk,
                                 const xbus::vehicle::ident_s &ident,
                                 const QString &callsign)
    : ProtocolBase(vehicles, callsign)
    , vehicles(vehicles)
{
    setIcon(squawk ? "drone" : "chip");

    m_uid = QByteArray(reinterpret_cast<const char *>(ident.uid), sizeof(xbus::vehicle::uid_t))
                .toHex()
                .toUpper();

    storage = new VehiclesStorage(this);

    nodes = new ProtocolNodes(this);
    telemetry = new ProtocolTelemetry(this);

    updateIdent(squawk, ident, callsign);

    // stream type and status
    setValue(streamTypeText());
    onlineTimer.setSingleShot(true);
    onlineTimer.setInterval(7000);
    connect(&onlineTimer, &QTimer::timeout, this, [this]() { updateStreamType(OFFLINE); });

    time_telemetry.start();
    time_xpdr.start();
    connect(this, &ProtocolVehicle::xpdrData, this, [this]() { updateStreamType(XPDR); });

    //downlink request timer
    if (isIdentified()) {
        telemetryReqTimer.setInterval(1000);
        connect(&telemetryReqTimer, &QTimer::timeout, this, &ProtocolVehicle::requestTelemetry);
        connect(this, &Fact::activeChanged, this, &ProtocolVehicle::updateActive);
    }

    if (!isReplay()) {
        storage->saveVehicleInfo();
    }
}

void ProtocolVehicle::dbKeyFound(quint64 key)
{
    m_dbKey = key;
}
void ProtocolVehicle::dbConfigInfoFound(QVariantMap info)
{
    m_dbConfigInfo = info;
    emit dbConfigInfoChanged();
}
void ProtocolVehicle::dbSetConfigHash(QString hash)
{
    m_dbConfigHash = hash;
}

const xbus::vehicle::ident_s &ProtocolVehicle::ident() const
{
    return m_ident;
}
void ProtocolVehicle::updateIdent(const xbus::vehicle::squawk_t &squawk,
                                  const xbus::vehicle::ident_s &ident,
                                  const QString &callsign)
{
    if (m_squawk != squawk) {
        m_squawk = squawk;
        emit squawkChanged();
    }

    if (memcmp(&m_ident, &ident, sizeof(ident)) != 0) {
        m_ident = ident;
        emit identChanged();
    }

    setName(callsign.toLower());
    setTitle(callsign);

    setEnabled(!isReplay());
}

bool ProtocolVehicle::match(const xbus::vehicle::squawk_t &squawk) const
{
    return squawk == m_squawk;
}
bool ProtocolVehicle::match(const xbus::vehicle::uid_t &uid) const
{
    return memcmp(uid, m_ident.uid, sizeof(xbus::vehicle::uid_t)) == 0;
}
bool ProtocolVehicle::match(const xbus::vehicle::ident_s &ident) const
{
    return memcmp(&ident, &m_ident, sizeof(xbus::vehicle::ident_s)) == 0;
}

void ProtocolVehicle::updateStreamType(StreamType type)
{
    if (type != OFFLINE) {
        onlineTimer.start();

        switch (type) {
        default:
            break;
        case TELEMETRY:
            time_telemetry.start();
            break;
        case NMT:
        case DATA:
            if (time_telemetry.elapsed() < 2000 || time_xpdr.elapsed() < 3000)
                return;
            break;
        }
    }
    if (m_streamType != type) {
        m_streamType = type;
        setValue(streamTypeText());
        emit streamTypeChanged();
    }
}
void ProtocolVehicle::updateActive()
{
    if (active() && isIdentified())
        telemetryReqTimer.start();
    else
        telemetryReqTimer.stop();
}

void ProtocolVehicle::downlink(ProtocolStreamReader &stream)
{
    if (stream.available() < xbus::pid_s::psize()) {
        qWarning() << "size" << stream.dump();
        return;
    }

    stream.trim();

    xbus::pid_s pid;
    pid.read(&stream);

    if (pid.uid > mandala::uid_max) {
        qWarning() << "wrong pid" << pid.uid << stream.dump();
        return;
    }

    if (mandala::cmd::env::match(pid.uid)) {
        emit receivedCmdEnvPacket(pid.uid);
    }

    if (mandala::cmd::env::nmt::match(pid.uid)) {
        updateStreamType(NMT);
        nodes->downlink(pid, stream);
        //forward to local nodes too
        if (!isLocal()
            && (pid.uid == mandala::cmd::env::nmt::search::uid
                || pid.uid == mandala::cmd::env::nmt::ident::uid
                || pid.uid == mandala::cmd::env::nmt::file::uid)) {
            stream.reset();
            vehicles->local->trace_downlink(pid);
            vehicles->local->downlink(stream);
        }
        return;
    }

    trace_downlink(pid);

    switch (pid.uid) {
    default:
        telemetry->downlink(pid, stream);
        return;
    case mandala::cmd::env::vcp::rx::uid:
        if (stream.available() > 1) {
            uint8_t port_id = stream.read<uint8_t>();
            trace_downlink(ProtocolTraceItem::DATA, QString::number(port_id));
            trace_downlink(stream.payload());
            updateStreamType(DATA);
            emit serialRxData(port_id, stream.payload());
            return;
        }
        qWarning() << "Empty serial RX data received";
        break;
    case mandala::cmd::env::vcp::tx::uid:
        if (stream.available() > 1) {
            uint8_t port_id = stream.read<uint8_t>();
            trace_downlink(ProtocolTraceItem::DATA, QString::number(port_id));
            trace_downlink(stream.payload());
            updateStreamType(DATA);
            emit serialTxData(port_id, stream.payload());
            return;
        }
        qWarning() << "Empty serial TX data received";
        break;
    case mandala::cmd::env::mission::data::uid:
        updateStreamType(DATA);
        break;
    case mandala::cmd::env::script::jsexec::uid:
        if (stream.available() > 2) {
            QString script = stream.payload().trimmed();
            if (!script.isEmpty()) {
                updateStreamType(DATA);
                emit jsexecData(script);
                return;
            }
        }
        qWarning() << "Empty jsexec data received" << stream.dump();
        break;
    case mandala::cmd::env::stream::calib::uid:
        if (stream.available() > sizeof(mandala::uid_t)) {
            mandala::uid_t uid;
            stream >> uid;
            updateStreamType(DATA);
            emit calibrationData(uid, stream.payload());
            return;
        }
        qWarning() << "Empty calibration data received";
        break;
    }
    inc_errcnt();
}

void ProtocolVehicle::send(const QByteArray packet)
{
    vehicles->send(m_squawk, packet);
}

void ProtocolVehicle::requestTelemetry()
{
    ostream.req(mandala::cmd::env::telemetry::data::uid);
    send(ostream.toByteArray());
}
void ProtocolVehicle::vmexec(QString func)
{
    func = func.simplified().trimmed();
    if (func.isEmpty())
        return;
    ostream.req(mandala::cmd::env::script::vmexec::uid);
    ostream.append(func.toUtf8());
    send(ostream.toByteArray());
}
void ProtocolVehicle::sendSerial(quint8 portID, QByteArray data)
{
    if (data.isEmpty())
        return;
    ostream.req(mandala::cmd::env::vcp::tx::uid);
    ostream.write<uint8_t>(portID);
    ostream.append(data);
    send(ostream.toByteArray());
}
void ProtocolVehicle::requestCalibrationData(mandala::uid_t uid, QByteArray data)
{
    ostream.req(mandala::cmd::env::stream::calib::uid);
    ostream << uid;
    ostream.append(data);
    send(ostream.toByteArray());
}

//---------------------------------------
// PROPERTIES
//---------------------------------------

xbus::vehicle::squawk_t ProtocolVehicle::squawk() const
{
    return m_squawk;
}
QString ProtocolVehicle::squawkText() const
{
    return QString("%1").arg(squawk(), 4, 16, QChar('0')).toUpper();
}
QString ProtocolVehicle::uid() const
{
    return m_uid;
}
ProtocolVehicle::StreamType ProtocolVehicle::streamType(void) const
{
    return m_streamType;
}
QString ProtocolVehicle::streamTypeText() const
{
    return QMetaEnum::fromType<StreamType>().valueToKey(streamType());
}
uint ProtocolVehicle::errcnt(void) const
{
    return m_errcnt;
}
void ProtocolVehicle::setErrcnt(const uint &v)
{
    if (m_errcnt == v)
        return;
    m_errcnt = v;
    emit errcntChanged();
}
void ProtocolVehicle::inc_errcnt()
{
    m_errcnt++;
    emit errcntChanged();
}

bool ProtocolVehicle::isLocal() const
{
    return squawk() == 0 && ident().flags.gcs == 0;
}
bool ProtocolVehicle::isReplay() const
{
    return squawk() == 0 && ident().flags.gcs == 1;
}
bool ProtocolVehicle::isIdentified() const
{
    return !(isLocal() || isReplay());
}
bool ProtocolVehicle::isGroundControl() const
{
    return isIdentified() && ident().flags.gcs == 1;
}
