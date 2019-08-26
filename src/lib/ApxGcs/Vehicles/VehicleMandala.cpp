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
#include "VehicleMandala.h"
#include "Vehicle.h"
#include "VehicleMandalaFact.h"
#include <ApxLog.h>
//=============================================================================
VehicleMandala::VehicleMandala(Vehicle *parent)
    : Fact(parent, "mandala", "Mandala", tr("Vehicle data tree"), Group)
    , vehicle(parent)
    , m_errcnt(0)
{
    setIcon("hexagon-multiple");

    //create facts
    DictMandala *dict = new DictMandala();
    special = dict->special;
    constants = dict->constants;
    for (int i = 0; i < dict->items.size(); ++i) {
        const DictMandala::Entry &d = dict->items.at(i);
        Flags t;
        switch (d.type) {
        default:
            t = Float;
            break;
        case QMetaType::QStringList:
            t = Enum;
            break;
        case QMetaType::Double:
            t = Float;
            break;
        case QMetaType::Int:
            t = Int;
            break;
        case QMetaType::Bool:
            t = Bool;
            break;
        case QMetaType::QVector2D:
            t = Float;
            break;
        case QMetaType::QVector3D:
            t = Float;
            break;
        }
        VehicleMandalaFact *f = registerFact(d.id, d.name, d.descr, d.units, t);
        if (!d.opts.isEmpty())
            f->setEnumStrings(d.opts);
        f->setPrecision(getPrecision(d));
        f->setColor(getColor(d));
        if (vehicle->protocol) {
            connect(f,
                    &VehicleMandalaFact::sendValueUpdate,
                    vehicle->protocol->telemetry,
                    &ProtocolTelemetry::sendValue);
            connect(f,
                    &VehicleMandalaFact::sendValueRequest,
                    vehicle->protocol->telemetry,
                    &ProtocolTelemetry::sendValueRequest);
            connect(f, &VehicleMandalaFact::sendValueUpdate, vehicle, [this, f]() {
                vehicle->recordUplink(f);
            });
        }
    }
    delete dict;

    //quick facts
    f_gps_lat = factByName("gps_lat");
    f_gps_lon = factByName("gps_lon");
    f_altitude = factByName("altitude");
    f_gSpeed = factByName("gSpeed");
    f_course = factByName("course");
    f_yaw = factByName("yaw");
    f_mode = factByName("mode");

    //protocols
    if (vehicle->protocol) {
        connect(vehicle->protocol, &ProtocolVehicle::xpdrData, this, &VehicleMandala::xpdrData);
        connect(vehicle->protocol->telemetry,
                &ProtocolTelemetry::mandalaValueReceived,
                this,
                &VehicleMandala::mandalaValueReceived);
    }
}
//=============================================================================
VehicleMandalaFact *VehicleMandala::registerFact(
    quint16 id, const QString &name, const QString &descr, const QString &units, Flags flags)
{
    VehicleMandalaFact *f = new VehicleMandalaFact(this, id, name, "", descr, units, flags);
    factByIdMap[id] = f;
    allFacts.append(f);
    names.append(name);
    return f;
}
int VehicleMandala::getPrecision(const DictMandala::Entry &i)
{
    switch (i.type) {
    default:
        break;
    case QMetaType::Int:
    case QMetaType::Bool:
    case QMetaType::QStringList:
        /*case vt_byte:
    case vt_uint:
    case vt_flag:
    case vt_enum:
    case vt_void:
    case vt_idx:*/
        return 0;
    }
    if (i.name.contains("lat") || i.name.contains("lon"))
        return 8;
    if (i.name == "ldratio")
        return 2;
    if (i.name.startsWith("ctr"))
        return 3;
    if (i.units == "0..1")
        return 3;
    if (i.units == "-1..0..+1")
        return 3;
    if (i.units == "deg")
        return 2;
    if (i.units == "deg/s")
        return 2;
    if (i.units == "m")
        return 2;
    if (i.units == "m/s")
        return 2;
    if (i.units == "m/s2")
        return 2;
    if (i.units == "a.u.")
        return 2;
    if (i.units == "v")
        return 2;
    if (i.units == "A")
        return 3;
    if (i.units == "C")
        return 1;
    return 6;
}
QColor VehicleMandala::getColor(const DictMandala::Entry &i)
{
    uint varmsk = i.id;
    QString sn = i.name;
    uint ci = 0;
    if (i.type == QMetaType::QVector2D || i.type == QMetaType::QVector3D)
        ci = (varmsk >> 8) + 1;
    QColor c(Qt::cyan);
    if (sn.startsWith("ctr_")) {
        if (sn.contains("ailerons"))
            c = QColor(Qt::red).lighter();
        else if (sn.contains("elevator"))
            c = QColor(Qt::green).lighter();
        else if (sn.contains("throttle"))
            c = QColor(Qt::blue).lighter();
        else if (sn.contains("rudder"))
            c = QColor(Qt::yellow).lighter();
        else if (sn.contains("collective"))
            c = QColor(Qt::darkCyan);
        else
            c = QColor(Qt::magenta).darker();
    } else if (sn.startsWith("rc_")) {
        if (sn.contains("roll"))
            c = QColor(Qt::darkRed);
        else if (sn.contains("pitch"))
            c = QColor(Qt::darkGreen);
        else if (sn.contains("throttle"))
            c = QColor(Qt::darkBlue);
        else if (sn.contains("yaw"))
            c = QColor(Qt::darkYellow);
        else
            c = QColor(Qt::magenta).lighter();
    } else if (sn.startsWith("ctrb_"))
        c = Qt::magenta;
    else if (sn.startsWith("user")) {
        if (sn.endsWith("r1"))
            c = QColor(Qt::red).lighter();
        else if (sn.endsWith("r2"))
            c = QColor(Qt::green).lighter();
        else if (sn.endsWith("r3"))
            c = QColor(Qt::blue).lighter();
        else if (sn.endsWith("r4"))
            c = QColor(Qt::yellow).lighter();
        else if (sn.endsWith("r5"))
            c = QColor(Qt::cyan).lighter();
        else if (sn.endsWith("r6"))
            c = QColor(Qt::magenta).lighter();
        else
            c = QColor(Qt::cyan).lighter();
    } else if (sn.startsWith("altitude"))
        c = QColor(Qt::red).lighter(170);
    else if (sn.startsWith("vspeed"))
        c = QColor(Qt::green).lighter(170);
    else if (sn.startsWith("airspeed"))
        c = QColor(Qt::blue).lighter(170);
    else if (i.type == QMetaType::Bool || i.type == QMetaType::QStringList)
        c = QColor(Qt::blue).lighter();
    else {
        if (ci == 1)
            c = Qt::red;
        else if (ci == 2)
            c = Qt::green;
        else if (ci == 3)
            c = Qt::yellow;
        if (sn.startsWith("cmd_"))
            c = c.lighter();
    }
    return c;
}
//=============================================================================
//=============================================================================
uint VehicleMandala::errcnt(void) const
{
    return m_errcnt;
}
bool VehicleMandala::setErrcnt(const uint &v)
{
    if (m_errcnt == v)
        return false;
    m_errcnt = v;
    //m->dl_errcnt=v;
    emit errcntChanged();
    return true;
}
//=============================================================================
QVariant VehicleMandala::valueById(quint16 id) const
{
    auto f = factById(id);
    if (f)
        return f->value();
    else
        return QVariant();
}
bool VehicleMandala::setValueById(quint16 id, const QVariant &v)
{
    auto f = factById(id);
    if (f)
        return f->setValue(v);

    return false;
}
VehicleMandalaFact *VehicleMandala::factById(quint16 id, uint16_t msb) const
{
    return factByIdMap.value(static_cast<quint16>(id | (msb << 8)), nullptr);
}
QVariant VehicleMandala::valueByName(const QString &vname) const
{
    auto f = factByName(vname);
    if (f)
        return f->value();
    else
        return QVariant();
}
VehicleMandalaFact *VehicleMandala::factByName(const QString &vname) const
{
    VehicleMandalaFact *f = static_cast<VehicleMandalaFact *>(child(vname));
    if (!f) {
        apxConsoleW() << "Vehicle mandala fact not found:" << vname;
    }
    return f;
}
//=============================================================================
void VehicleMandala::xpdrData(const ProtocolVehicles::XpdrData &xpdr)
{
    f_gps_lat->setValueLocal(xpdr.lat);
    f_gps_lon->setValueLocal(xpdr.lon);
    f_altitude->setValueLocal(xpdr.alt);
    f_gSpeed->setValueLocal(xpdr.gSpeed);
    f_course->setValueLocal(xpdr.course);
    f_yaw->setValueLocal(xpdr.course);
    f_mode->setValueLocal(xpdr.mode);
}
//=============================================================================
void VehicleMandala::mandalaValueReceived(quint16 id, double v)
{
    VehicleMandalaFact *f = factByIdMap.value(id);
    if (!f)
        return;
    f->setValueLocal(v);
}
//=============================================================================
