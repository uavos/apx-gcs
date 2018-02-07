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
#include "Vehicle.h"
#include "Vehicles.h"
#include "VehicleMandala.h"
#include "VehicleNmtManager.h"
#include "VehicleRecorder.h"
#include "VehicleWarnings.h"
#include "Nodes.h"
#include "VehicleMission.h"
#include "Mandala.h"
//=============================================================================
Vehicle::Vehicle(Vehicles *parent, QString callsign, quint16 squawk, QByteArray uid, VehicleClass vclass)
  : Fact(vclass>=LOCAL?parent:parent->f_list,callsign,callsign,"",GroupItem,NoData),
    uid(uid),
    m_streamType(OFFLINE),
    m_squawk(squawk),
    m_callsign(callsign),
    m_vehicleClass(vclass)
{
  setSection(parent->title());
  setIcon(vclass==LOCAL?"chip":vclass==REPLAY?"play-circle":"drone");

  //requests manager
  if(vclass!=REPLAY){
    nmtManager=new VehicleNmtManager(this);
    connect(nmtManager,&VehicleNmtManager::sendUplink,this,&Vehicle::sendUplink);
    connect(this,&Vehicle::nmtReceived,nmtManager,&VehicleNmtManager::nmtReceived);
  }

  connect(this,&Vehicle::callsignChanged,this,&Vehicle::updateTitle);
  connect(this,&Vehicle::streamTypeChanged,this,&Vehicle::updateStatus);

  f_select=new FactAction(this,"select",tr("Select"),tr("Make this vehicle active"),FactAction::NormalAction,"select");
  connect(f_select,&FactAction::triggered,this,[=](){ parent->selectVehicle(this); });
  connect(parent,&Vehicles::vehicleSelected,this,[=](Vehicle *v){ f_select->setEnabled(v!=this); });

  f_mandala=new VehicleMandala(this);
  f_nodes=new Nodes(this);
  f_mission=new VehicleMission(this);
  f_recorder=new VehicleRecorder(this);
  f_warnings=new VehicleWarnings(this);

  //if(isReplay())f_recorder->setVisible(false);

  //datalink
  if(!isReplay()){
    connect(this,&Vehicle::sendUplink,this,[=](const QByteArray &ba){
      parent->vehicleSendUplink(this,ba);
      f_recorder->recordUplink(ba);
    });
  }


  connect(parent,&Vehicles::vehicleSelected,this,[=](Vehicle *v){ setActive(v==this); });


  onlineTimer.setSingleShot(true);
  onlineTimer.setInterval(7000);
  connect(&onlineTimer,&QTimer::timeout,this,[=](){ setStreamType(OFFLINE); });

  updateStatus();

  //register JS new vehicles instantly
  connect(this,&Vehicle::nameChanged,this,[=](){FactSystem::instance()->jsSync(this);});
  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void Vehicle::updateTitle()
{
  setName(callsign());
  setTitle(callsign());
}
void Vehicle::updateStatus()
{
  setStatus(streamTypeText());
  f_mandala->setStatus(status());
}
//=============================================================================
bool Vehicle::isLocal() const
{
  return vehicleClass()==LOCAL;
}
bool Vehicle::isReplay() const
{
  return vehicleClass()==REPLAY;
}
void Vehicle::setReplay(bool v)
{
  if(v){
    setVisible(true);
    setStreamType(TELEMETRY);
    onlineTimer.start();
  }else if(isReplay()){
    onlineTimer.stop();
    setStreamType(OFFLINE);
  }
}
//=============================================================================
QString Vehicle::streamTypeText() const
{
  return QMetaEnum::fromType<StreamType>().valueToKey(streamType());
}
QString Vehicle::vehicleClassText() const
{
  return QMetaEnum::fromType<VehicleClass>().valueToKey(vehicleClass());
}
QString Vehicle::squawkText() const
{
  return QString::number(squawk(),16).toUpper();
}
//=============================================================================
void Vehicle::downlinkReceived(const QByteArray &packet)
{
  if(isReplay()) return;
  if(f_nodes->unpackService(packet)){
    emit nmtReceived(packet);
    if(telemetryTime.elapsed()>2000 && xpdrTime.elapsed()>3000)
      setStreamType(SERVICE);
  }else if(f_mandala->unpackTelemetry(packet)){
    f_recorder->recordDownlink(packet);
    setStreamType(TELEMETRY);
    telemetryTime.start();
  }else if(f_mandala->unpackData(packet)){
    if(telemetryTime.elapsed()>2000 && xpdrTime.elapsed()>3000)
      setStreamType(DATA);
  }else if(f_mission->unpackMission(packet)){
    if(telemetryTime.elapsed()>2000 && xpdrTime.elapsed()>3000)
      setStreamType(DATA);
  }else return;
  onlineTimer.start();
}
//=============================================================================
void Vehicle::xpdrReceived(const QByteArray &data)
{
  if(isReplay()) return;
  if(f_mandala->unpackXPDR(data)){
    f_recorder->recordDownlink(data);
    setStreamType(XPDR);
    xpdrTime.start();
  }else return;
  onlineTimer.start();
}
//=============================================================================
void Vehicle::vmexec(QString func)
{
  emit sendUplink(QByteArray().append((unsigned char)idx_vmexec).append(func));
}
//=============================================================================
void Vehicle::sendSerial(quint8 portID,QByteArray data)
{
  emit sendUplink(QByteArray().append((unsigned char)idx_data).append((unsigned char)portID).append(data));
}
void FactSystemJS::sleep(quint16 ms)
{
  QEventLoop loop;
  QTimer::singleShot(ms,&loop,SLOT(quit()));
  loop.exec();
}
//=============================================================================
void Vehicle::requestMission()
{
  emit sendUplink(QByteArray().append((unsigned char)idx_mission));
}
//=============================================================================
QString Vehicle::fileTitle() const
{
  QString s=confTitle();
  if(s.isEmpty())return title();
  return s;
}
QString Vehicle::confTitle() const
{
  QString confName;
  foreach(NodeItem *node,f_nodes->snMap.values()){
    if(!node->title().endsWith(".shiva"))continue;
    confName=node->status().trimmed();
    if(!confName.isEmpty())break;
  }
  return confName;
}
//=============================================================================
//=============================================================================
Vehicle::StreamType Vehicle::streamType(void) const
{
  return m_streamType;
}
void Vehicle::setStreamType(const StreamType v)
{
  if(m_streamType==v)return;
  m_streamType=v;
  emit streamTypeChanged();
}
quint16 Vehicle::squawk(void) const
{
  return m_squawk;
}
void Vehicle::setSquawk(const quint16 v)
{
  if(m_squawk==v)return;
  m_squawk=v;
  emit squawkChanged();
}
QString Vehicle::callsign(void) const
{
  return m_callsign;
}
void Vehicle::setCallsign(const QString &v)
{
  if(m_callsign==v)return;
  m_callsign=v;
  emit callsignChanged();
}
Vehicle::VehicleClass Vehicle::vehicleClass(void) const
{
  return m_vehicleClass;
}
void Vehicle::setVehicleClass(const VehicleClass v)
{
  if(m_vehicleClass==v)return;
  m_vehicleClass=v;
  emit vehicleClassChanged();
}
//=============================================================================
