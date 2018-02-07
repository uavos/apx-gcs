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
#ifndef Vehicle_H
#define Vehicle_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
#include "VehicleMandala.h"
#include "VehicleNmtManager.h"
class Vehicles;
class Nodes;
class VehicleMission;
class VehicleRecorder;
class VehicleWarnings;
//=============================================================================
class Vehicle: public Fact
{
  Q_OBJECT
  Q_ENUMS(VehicleClass)
  Q_ENUMS(StreamType)

  Q_PROPERTY(QString callsign READ callsign NOTIFY callsignChanged)
  Q_PROPERTY(quint16 squawk READ squawk NOTIFY squawkChanged)

  Q_PROPERTY(VehicleClass vehicleClass READ vehicleClass NOTIFY vehicleClassChanged)
  Q_PROPERTY(QString vehicleClassText READ vehicleClassText NOTIFY vehicleClassChanged)

  Q_PROPERTY(StreamType streamType READ streamType NOTIFY streamTypeChanged)
  Q_PROPERTY(QString streamTypeText READ streamTypeText NOTIFY streamTypeChanged)

public:
  enum VehicleClass {
    //must match the IDENT::_vclass type
    UAV =0,
    GCU,

    //internal use
    LOCAL=100,
    REPLAY
  };
  Q_ENUM(VehicleClass)

  enum StreamType {
    OFFLINE =0,
    SERVICE,
    DATA,
    XPDR,
    TELEMETRY
  };
  Q_ENUM(StreamType)

  explicit Vehicle(Vehicles *parent, QString callsign, quint16 squawk, QByteArray uid, VehicleClass vclass);

  VehicleMandala  *f_mandala;
  Nodes           *f_nodes;
  VehicleMission  *f_mission;
  VehicleRecorder *f_recorder;
  VehicleWarnings *f_warnings;

  FactAction * f_select;

  VehicleNmtManager *nmtManager;

  QByteArray uid;

  QString vehicleClassText() const;
  QString streamTypeText() const;
  QString squawkText() const;


  QString fileTitle() const; //name based on Vehicle title and nodes shiva comment
  QString confTitle() const;

  bool isLocal() const;
  bool isReplay() const;

  void setReplay(bool v);
private:
  QTimer onlineTimer;
  QTime telemetryTime;
  QTime xpdrTime;
  QTime replayTime;

private slots:
  void updateTitle();
  void updateStatus();

  //data connection
public slots:
  void xpdrReceived(const QByteArray &data);
  void downlinkReceived(const QByteArray &packet);
signals:
  void sendUplink(const QByteArray &packet);
  void nmtReceived(const QByteArray &packet);

  //provided methods
public slots:
  void vmexec(QString func);
  void sendSerial(quint8 portID, QByteArray data);
  void requestMission();


  //---------------------------------------
  // PROPERTIES
public:
  StreamType streamType(void) const;
  void setStreamType(const StreamType v);

  quint16 squawk(void) const;
  void setSquawk(const quint16 v);

  QString callsign(void) const;
  void setCallsign(const QString &v);

  VehicleClass vehicleClass(void) const;
  void setVehicleClass(const VehicleClass v);

protected:
  StreamType m_streamType;
  quint16 m_squawk;
  QString m_callsign;
  VehicleClass m_vehicleClass;

signals:
  void streamTypeChanged();
  void squawkChanged();
  void callsignChanged();
  void vehicleClassChanged();


};
//=============================================================================
#endif

