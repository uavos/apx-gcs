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
#include <QtCore>
#include "FactSystem.h"
#include "VehicleMandalaFact.h"
class Vehicle;
class Mandala;
//=============================================================================
class VehicleMandala: public Fact
{
  Q_OBJECT

  Q_PROPERTY(QByteArray md5 READ md5 WRITE setMd5 NOTIFY md5Changed)
  Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged)

public:
  explicit VehicleMandala(Vehicle *parent);
  ~VehicleMandala();

  QHash<QString,QVariant> constants; // <name,value> enums in form varname_ENUM
  QHash<QString,quint16> special; // <name,id>
  QStringList names;
  QList<quint16> ids;

  QList<VehicleMandalaFact*> allFacts;

  QVariant valueById(quint16 id) const;
  bool setValueById(quint16 id,const QVariant &v);

  VehicleMandalaFact * factById(quint16 id) const;

  QVariant valueByName(const QString &vname) const;
  VehicleMandalaFact * factByName(const QString &vname) const;

  bool unpackData(const QByteArray &ba);
  bool unpackTelemetry(const QByteArray &ba);
  bool unpackXPDR(const QByteArray &ba);

private:
  Mandala *m;
  VehicleMandalaFact * registerFact(quint16 id, DataType dataType, const QString &name, const QString &descr, const QString &units);
  QMap<quint16,VehicleMandalaFact*> idMap;

  void collectValues();

  //EXPORTED
signals:
  void dataReceived(uint id);
  void serialReceived(uint portNo,const QByteArray &ba);

  //data comm
signals:
  void sendUplink(const QByteArray &ba);



  //---------------------------------------
  // PROPERTIES
public:
  QByteArray md5(void) const;
  bool setMd5(const QByteArray &v);
  uint errcnt(void) const;
  bool setErrcnt(const uint &v);

protected:
  QByteArray m_md5;
  uint m_errcnt;

signals:
  void md5Changed();
  void errcntChanged();

};
//=============================================================================
#endif

