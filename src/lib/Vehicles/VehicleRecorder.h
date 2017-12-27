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
#ifndef VehicleRecorder_H
#define VehicleRecorder_H
#include <QtCore>
#include <TelemetryDB.h>
#include <FactSystem.h>
#include "MandalaValue.h"
class Vehicle;
class VehicleMandalaFact;
class QProgressBar;
//=============================================================================
class VehicleRecorder : public Fact
{
  Q_OBJECT
public:
  explicit VehicleRecorder(Vehicle *parent);

  bool recDisable;  //externally disable recording (f.ex. by player)

  TelemetryDB * _db;
private:
  Vehicle *vehicle;

  //database
  bool dbCheckRecord();
  void dbDownlinkWrite();
  void dbUplinkWrite(quint64 fieldID, const QVariant &v);

  quint64 recTelemetryID;
  QList<double> recValues;


  //auto recorder
  bool checkAutoRecord(void);
  VehicleMandalaValue<idx_mode,int> v_mode;
  VehicleMandalaValue<idx_stage,int> v_stage;
  bool recTrigger;
  QTimer recTimeUpdateTimer,recStopTimer;

  //timestamp
  VehicleMandalaValue<idx_dl_timestamp,uint> v_dl_timestamp;
  uint dl_timestamp_s,dl_timestamp_t0;
  QTime uplinkTime;

  quint64 recTimestamp;
  quint64 getDataTimestamp();


private slots:
  void recTimeUpdate(void);
  void updateStatus();

public slots:
  void recordDownlink(const QByteArray &data);
  void recordUplink(const QByteArray &data);
  void recordEvent(const QString &name, const QString &value, bool uplink=false, const QByteArray &data=QByteArray());

  //PROPERTIES
public:
  Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)
  bool recording() const;
  Q_PROPERTY(quint64 recTime READ recTime NOTIFY recTimeChanged)
  quint64 recTime() const;
private:
  bool m_recording;
  quint64 m_recTime;
  void setRecTime(quint64 v,bool forceUpdate=false);
signals:
  void recordingChanged();
  void recTimeChanged();
public slots:
  Q_INVOKABLE void setRecording(bool v);
  Q_INVOKABLE void reset(void);
};
//=============================================================================
#endif // FLIGHTDATAFILE_H
