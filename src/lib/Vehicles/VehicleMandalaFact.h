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
#ifndef VehicleMandalaFact_H
#define VehicleMandalaFact_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
class Mandala;
class VehicleMandala;
//=============================================================================
class VehicleMandalaFact: public Fact
{
  Q_OBJECT
  Q_PROPERTY(QString units READ units CONSTANT)

public:
  explicit VehicleMandalaFact(
      VehicleMandala *parent,
      Mandala *m,
      quint16 id,
      DataType dataType,
      const QString &name,
      const QString &title,
      const QString &descr,
      const QString &units
      );

  bool setValue(const QVariant &v); //override
  bool setValueLocal(const QVariant &v);

  double unpackedValue();

  Q_INVOKABLE quint16 id() {return m_id;}

  uint _vtype;
private:
  VehicleMandala *vehicle;
  Mandala *m;
  quint16 m_id;

  void *_value_ptr;
  double _unpackedValue;

  bool pack();
  QByteArray packed;
  uint8_t tmp[32];

  //local value change by telemetry delay
  QTimer loadValueTimer;
  QTimer sendValueTimer;
  QTime sendValueTime;
  int setValueCnt;

private slots:
  void loadValueDo();


  //---------------------------------------
  // PROPERTIES
public:
  QString units(void) const;

protected:
  QString m_units;

public slots:
  void saveValue();
  void loadValue();

signals:
  void sendUplink(const QByteArray &ba);

};
//=============================================================================
#endif

