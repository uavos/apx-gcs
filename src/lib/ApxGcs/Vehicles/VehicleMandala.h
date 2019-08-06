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
#ifndef VehicleMandala_H
#define VehicleMandala_H
//=============================================================================
#include "VehicleMandalaFact.h"
#include <Dictionary/DictMandala.h>
#include <Fact/Fact.h>
#include <Protocols/ProtocolVehicles.h>
#include <QtCore>
class Vehicle;
//=============================================================================
class VehicleMandala : public Fact
{
    Q_OBJECT

    Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged)

public:
    explicit VehicleMandala(Vehicle *parent);

    Vehicle *vehicle;

    QHash<QString, QVariant> constants; // <name,value> enums in form varname_ENUM
    QHash<QString, quint16> special;    // <name,id>
    QStringList names;

    QList<VehicleMandalaFact *> allFacts;

    QVariant valueById(quint16 id) const;
    bool setValueById(quint16 id, const QVariant &v);

    VehicleMandalaFact *factById(quint16 id, uint16_t msb = 0) const;

    QVariant valueByName(const QString &vname) const;
    VehicleMandalaFact *factByName(const QString &vname) const;

private:
    VehicleMandalaFact *registerFact(quint16 id,
                                     const QString &name,
                                     const QString &descr,
                                     const QString &units,
                                     Fact::Flags flags);
    QMap<quint16, VehicleMandalaFact *> factByIdMap;

    int getPrecision(const DictMandala::Entry &i);
    QColor getColor(const DictMandala::Entry &i);

    //quick facts access
    VehicleMandalaFact *f_gps_lat;
    VehicleMandalaFact *f_gps_lon;
    VehicleMandalaFact *f_altitude;
    VehicleMandalaFact *f_gSpeed;
    VehicleMandalaFact *f_course;
    VehicleMandalaFact *f_yaw;
    VehicleMandalaFact *f_mode;

    //protocols connection
private slots:
    void xpdrData(const ProtocolVehicles::XpdrData &xpdr);
    void mandalaValueReceived(quint16 id, double v);

    //---------------------------------------
    // PROPERTIES
public:
    uint errcnt(void) const;
    bool setErrcnt(const uint &v);

protected:
    uint m_errcnt;

signals:
    void errcntChanged();
};
//=============================================================================
#endif
