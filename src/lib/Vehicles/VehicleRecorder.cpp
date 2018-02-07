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
#include "VehicleRecorder.h"
#include <QProgressBar>
#include <QProgressDialog>
#include <QMessageBox>
#include <QApplication>
#include <math.h>
#include <cmath>
#include <cfloat>
#include "AppDirs.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
#include "Nodes.h"

#include "node.h"
#include "Mandala.h"
//=============================================================================
VehicleRecorder::VehicleRecorder(Vehicle *parent)
  :Fact(parent,"recorder",tr("Recorder"),tr("Telemetry data recorder"),FactItem,NoData),
  vehicle(parent),
  v_mode(vehicle),
  v_stage(vehicle),
  v_dl_timestamp(vehicle),
  dl_timestamp_s(0),
  dl_timestamp_t0(0),
  m_recording(false),
  m_recTime(0)
{
  setIcon("record-rec");

  //database
  _db = new TelemetryDB(this,QString("GCSVehicleRecorderSession_%1").arg(QString(vehicle->uid.toHex().toUpper())),vehicle);
  recTelemetryID=0;



  recTrigger=false;
  recTimeUpdateTimer.setSingleShot(true);
  recTimeUpdateTimer.setInterval(500);
  connect(&recTimeUpdateTimer,&QTimer::timeout,this,&VehicleRecorder::recTimeUpdate);

  recStopTimer.setSingleShot(true);
  connect(&recStopTimer,&QTimer::timeout,this,[=](){setRecording(false);});

  //setRecording(QSettings().value("recording",false).toBool());

  //status change (size/time)
  connect(this,&VehicleRecorder::recordingChanged,this,[=](){ setActive(recording()); });
  connect(this,&VehicleRecorder::recTimeChanged,this,&VehicleRecorder::updateStatus);
  updateStatus();

  if(!vehicle->isReplay()){
    connect(this,&Fact::triggered,this,[=](){setRecording(!recording());});
  }

  recordingChanged();
}
//=============================================================================
void VehicleRecorder::updateStatus()
{
  setStatus(FactSystem::timeToString(recTime(),true));
}
//=============================================================================
bool VehicleRecorder::dbCheckRecord()
{
  if(vehicle->isReplay())return false;
  checkAutoRecord();
  if(recTelemetryID)return true;
  recTimestamp=QDateTime::currentDateTime().toMSecsSinceEpoch();
  uplinkTime.start();
  //register telemetry file record
  recTelemetryID=_db->writeRecord(vehicle->uid.toHex().toUpper(),vehicle->callsign(),vehicle->confTitle(),recording(),recTimestamp);
  return recTelemetryID;
}
//=============================================================================
quint64 VehicleRecorder::getDataTimestamp()
{
  /*quint64 t=QDateTime::currentDateTime().toMSecsSinceEpoch();
  if(t<recTimestamp)t=0;
  else t-=recTimestamp;*/
  //snap to dl_timestamp
  if(!dl_timestamp_t0)dl_timestamp_t0=v_dl_timestamp;
  uint t=v_dl_timestamp-dl_timestamp_t0;

  if(dl_timestamp_s!=t){
    dl_timestamp_s=t;
    uplinkTime.start();
  }else t+=uplinkTime.elapsed();
  /*static quint64 dt_s=0,t_s=0;
  //dt test
  quint64 dt=t-t_s;
  t_s=t;
  if(dt_s!=dt && dt){
    dt_s=dt;
    qDebug()<<dt;
  }*/

  setRecTime(t/1000);
  return t;
}
//=============================================================================
void VehicleRecorder::dbDownlinkWrite()
{
  if(vehicle->isReplay())return;
  //collect changed facts
  QList<Fact*> facts;
  int i=-1;
  foreach (Fact *f, _db->recFacts.keys()) {
    i++;
    QVariant vv=f->value();
    double v=vv.toDouble();
    if(i<recValues.size()){
      if(recValues.at(i)==v){
        continue;
      }else{
        recValues[i]=v;
      }
    }else recValues.append(v);
    facts.append(f);
  }
  if(facts.isEmpty())return;
  if(!dbCheckRecord())return;
  _db->writeDownlink(recTelemetryID,getDataTimestamp(),facts);
}
//=============================================================================
void VehicleRecorder::dbUplinkWrite(quint64 fieldID, const QVariant &v)
{
  if(!dbCheckRecord())return;
  _db->writeField(recTelemetryID,getDataTimestamp(),fieldID,v,true);
}
//=============================================================================
//=============================================================================
void VehicleRecorder::recordDownlink(const QByteArray &data)
{
  if(data.size()<=bus_packet_size_hdr)return;
  dbDownlinkWrite(); //write all updated facts
}
//=============================================================================
void VehicleRecorder::recordUplink(const QByteArray &data)
{
  if(vehicle->isReplay())return;
  const _bus_packet &packet=*(_bus_packet*)data.data();
  VehicleMandalaFact *f=NULL;
  switch(packet.id){
    case idx_set: {
      if(data.size()<(bus_packet_size_hdr+2+1))return;
      f=vehicle->f_mandala->factById(packet.data[0]|packet.data[1]<<8);
    } break;
    default: {
      if(packet.id>=idxPAD){
        //regular var update
        f=vehicle->f_mandala->factById(packet.id);
      }else if(packet.id!=idx_service){
        //raw data
        QString vname=vehicle->f_mandala->special.key(packet.id,QString::number(packet.id));
        recordEvent("raw",vname,true,data);
      }
    }break;
  }
  if(f && _db->recFacts.contains(f)){
    dbUplinkWrite(_db->recFacts.value(f),f->value());
  }
}
//=============================================================================
void VehicleRecorder::recordEvent(const QString &name, const QString &value, bool uplink, const QByteArray &data)
{
  if(!dbCheckRecord())return;
  _db->writeEvent(recTelemetryID,getDataTimestamp(),name,value,uplink,data);
}
//=============================================================================
//=============================================================================






//=============================================================================
bool VehicleRecorder::checkAutoRecord(void)
{
  if(vehicle->streamType()==Vehicle::TELEMETRY) {
    if((v_mode==mode_TAKEOFF)&&(v_stage>=2)&&(v_stage<100)){
      if(!recTrigger){
        reset(); //restart
        setRecording(true);
        recTrigger=true;
      }
    }else recTrigger=false;
    if (recording()) {
      if((!recStopTimer.isActive()) && (v_mode==mode_LANDING)&&(v_stage>=250))
        recStopTimer.start(2000);
    }
  }
  if (!recording()){
    return false;
  }
  return true;
}
//=============================================================================
bool VehicleRecorder::recording() const
{
  return m_recording;
}
void VehicleRecorder::setRecording(bool v)
{
  recStopTimer.stop();
  if(m_recording==v)return;
  m_recording=v;
  reset();
  emit recordingChanged();
  if(!v){
    recTrigger=false;
  }
}
void VehicleRecorder::reset(void)
{
  recTelemetryID=0;
  dl_timestamp_s=0;
  dl_timestamp_t0=0;
  recValues.clear();
  setRecTime(0,true);
}
//=============================================================================
quint64 VehicleRecorder::recTime() const
{
  return m_recTime;
}
void VehicleRecorder::setRecTime(quint64 v, bool forceUpdate)
{
  if(m_recTime==v)return;
  m_recTime=v;
  if(forceUpdate){
    recTimeUpdateTimer.stop();
    recTimeUpdate();
    return;
  }
  if(!recTimeUpdateTimer.isActive())recTimeUpdateTimer.start();
}
void VehicleRecorder::recTimeUpdate(void)
{
  emit recTimeChanged();
}
//=============================================================================
