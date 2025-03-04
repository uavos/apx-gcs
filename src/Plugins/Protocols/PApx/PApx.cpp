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
#include "PApx.h"

#include "PApxFirmware.h"

#include <App/App.h>
#include <Mandala/Mandala.h>

PApx::PApx(Fact *parent)
    : PBase(parent, "papx", tr("APX Protocol"), tr("Standard data format for APX Autopilot"))
    , _req(this)
{
    // delayed requests timer
    _reqTimer.setInterval(500);
    connect(&_reqTimer, &QTimer::timeout, this, &PApx::request_next);

    connect(App::instance(), &App::loadingFinished, this, &PApx::updateLocal, Qt::QueuedConnection);
}

void PApx::updateLocal()
{
    m_local = new PApxUnit(this, "LOCAL", PUnit::UAV, 0, 0);
    m_firmware = new PApxFirmware(this);
    emit unit_available(m_local);
}

void PApx::process_downlink(QByteArray packet)
{
    if (!m_local) // not loaded yet
        return;

    if (static_cast<size_t>(packet.size()) > xbus::size_packet_max)
        return;

    PStreamReader stream(packet);

    if (stream.available() < xbus::pid_s::psize()) {
        qWarning() << "packet" << packet.toHex().toUpper();
        return;
    }
    xbus::pid_s pid;
    pid.read(&stream);
    stream.trim();

    trace_pid(pid);

    if (pid.eid == xbus::eid_none) {
        // is not unit wrapped format - forward to local
        m_local->process_downlink(pid, stream);
        return;
    }

    // unit addressed packets only

    // squawk must be present
    if (stream.available() <= sizeof(xbus::unit::squawk_t))
        return;
    const auto squawk = stream.read<xbus::unit::squawk_t>();
    const auto squawkText = PApx::squawkText(squawk);
    trace()->block(squawkText);
    stream.trim();

    if (pid.req) {
        // received uplink request from another GCS instance
        auto v = _squawk_map.value(squawk);
        if (!v)
            return;

        v->packetReceived(pid.uid);
        trace()->block(v->title().append(':'));
        trace()->tree();

        v->process_downlink(pid, stream);
        return;
    }

    if (pid.uid == xbus::cmd::unit::ident) {
        // received unit ident packet

        // UUID follows squawk
        if (stream.available() <= sizeof(xbus::unit::uid_t))
            return;
        xbus::unit::uid_t uid_raw;
        stream.read(uid_raw, sizeof(uid_raw));
        trace()->raw(uid_raw, "uid");

        if (stream.available() < xbus::unit::ident_s::psize())
            return;

        trace()->data(stream.payload());

        xbus::unit::ident_s ident;
        ident.read(&stream);

        const char *s = stream.read_string(stream.available());
        if (!s || stream.available() > 0)
            return;

        auto callsign = QString(s).trimmed();
        if (callsign.isEmpty()) {
            qWarning() << "callsign empty";
            callsign = "UAV";
        }

        if (!squawk) {
            //received zero SQUAWK
            assign_squawk(uid_raw);
            return;
        }

        QString uid = PApxUnit::uidText(&uid_raw);

        PUnit::UnitType type = ident.flags.gcs ? PUnit::GCS : PUnit::UAV;

        // lookup unit by uid
        PApxUnit *available = nullptr;
        for (auto c : facts()) {
            PApxUnit *i = static_cast<PApxUnit *>(c);
            if (i->uid() == uid) {
                available = i;
                break;
            }
        }

        // lookup unit by squawk
        PApxUnit *identified = _squawk_map.value(squawk);

        if (available) {
            available->packetReceived(pid.uid);
            if (!identified) {
                _squawk_map.insert(squawk, available);
                _req_ident.removeAll(squawk);
                qDebug() << "re-identified" << available->title();
            } else if (available != identified) {
                qWarning() << "duplicate squawk: " << squawkText;
                _squawk_map.remove(squawk);
                _squawk_map.remove(_squawk_map.key(available));
                _squawk_map.remove(_squawk_map.key(identified));
                assign_squawk(uid_raw);
                return;
            }
            // update matched by uid info from ident
            available->setTitle(callsign);
            available->setUnitType(type);
            available->setSquawk(squawk);
        } else {
            // unit not created
            if (identified) {
                qWarning() << "change squawk: " << PApx::squawkText(squawk);
                _squawk_map.remove(squawk);
                assign_squawk(uid_raw);
                return;
            } else {
                // add new identified unit
                qDebug() << "created" << callsign << PApx::squawkText(squawk);
                available = new PApxUnit(this, callsign, type, &uid_raw, squawk);
                _squawk_map.insert(squawk, available);
                _req_ident.removeAll(squawk);
                connect(available, &Fact::removed, this, [this, available]() {
                    _squawk_map.remove(_squawk_map.key(available));
                });
                emit unit_available(available);
                available->packetReceived(pid.uid);
            }
        }
        return;
    }

    // received some unit addressed data packet

    // VUID byte check follows
    if (stream.available() <= 1)
        return;
    uint8_t vuid_n;
    stream >> vuid_n;
    trace()->raw(vuid_n);
    stream.trim();

    auto v = _squawk_map.value(squawk);
    if (v) {
        if (!check_vuid(v, vuid_n, pid.seq))
            return;

        // vuid still ok
        v->packetReceived(pid.uid);
        trace()->block(v->title().append(':'));
        trace()->tree();
        v->process_downlink(pid, stream);
        return;
    }

    // new unit detected
    if (pid.uid == xbus::cmd::node::msg) {
        // allow messages from unknown units
        m_local->process_downlink(pid, stream);
    } else {
        trace()->data(stream.payload());
    }

    request_ident_schedule(squawk);
}
bool PApx::check_vuid(PApxUnit *v, uint8_t n, uint8_t seq)
{
    if (v->check_vuid(n, seq))
        return true;

    qWarning() << "VUID check failed";
    _squawk_map.remove(v->squawk());
    _squawk_map.remove(_squawk_map.key(v));
    assign_squawk(v->vuid());
    return false;
}

void PApx::request_ident_schedule(xbus::unit::squawk_t squawk)
{
    //qDebug() << squawkText(squawk);
    if (_req_ident.contains(squawk))
        return;

    _req_ident.append(squawk);
    if (!_reqTimer.isActive())
        request_next();
    _reqTimer.start();
}
void PApx::request_next()
{
    if (!_req_ident.isEmpty()) {
        request_ident(_req_ident.takeFirst());
        return;
    }
    _reqTimer.stop();
}

void PApx::request_ident(xbus::unit::squawk_t squawk)
{
    //qDebug() << squawkText(squawk);
    _req.request(xbus::cmd::unit::ident);
    _req.write<xbus::unit::squawk_t>(squawk);
    trace()->block(PApx::squawkText(squawk));
    _req.send();
}
void PApx::assign_squawk(const xbus::unit::uid_t &uid)
{
    //generate squawk
    xbus::unit::squawk_t squawk = 0xA5AD;
    xbus::unit::squawk_t tcnt = 32767;
    do {
        squawk = (static_cast<xbus::unit::squawk_t>(QRandomGenerator::global()->generate()) + tcnt)
                 ^ squawk;
        if (squawk < 100 || squawk > 0xff00)
            continue;
        if (_squawk_map.contains(squawk))
            continue;
        if (_squawk_blacklist.contains(squawk))
            continue;
        break;
    } while (tcnt--);
    if (!tcnt) {
        qDebug() << "Can't find new squawk for assignment";
        return;
    }
    _squawk_blacklist.append(squawk);
    if (_squawk_blacklist.size() > 1000)
        _squawk_blacklist.takeFirst();

    _req.request(xbus::cmd::unit::ident);
    _req.write<xbus::unit::squawk_t>(squawk);
    trace()->block(PApx::squawkText(squawk));

    _req.write(uid, sizeof(uid));
    trace()->raw(uid, "uid");

    _req.send();
}

void PApx::trace_pid(const xbus::pid_s &pid)
{
    if (!trace()->enabled())
        return;

    trace_uid(pid.uid);

    if (pid.eid)
        trace()->block(QString("+%1").arg(pid.eid));

    QString s;
    if (pid.req)
        s.append("Q");
    s.append(QString::number(static_cast<int>(pid.seq)));
    trace()->block(s);
}
void PApx::trace_uid(mandala::uid_t uid)
{
    if (!trace()->enabled())
        return;

    QString s;
    if (xbus::cmd::match(uid) || true) {
        s = QString("cmd:") + QString::number(uid - xbus::cmd::uid, 16).toUpper();
    } else {
        s = Mandala::meta(uid).path;
    }
    trace()->block(QString("$%1").arg(s));
}
