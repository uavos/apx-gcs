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
#include "Vehicles.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
#include "VehicleWarnings.h"
//---------------------------
// deprecate
#include "node.h"
#include "Mandala.h"
//=============================================================================
Vehicles * Vehicles::_instance=NULL;
Vehicles::Vehicles(FactSystem *parent)
  : Fact(parent,"vehicles",tr("Vehicles"),tr("Discovered vehicles"),SectionItem,NoData)
{
  _instance=this;

  setFlatModel(true);

  f_select=new Fact(this,"select",tr("Select vehicle"),tr("Change the active vehicle"),GroupItem,NoData);
  f_select->setSection(title());

  f_local=new Vehicle(this,"LOCAL",0,QByteArray(),Vehicle::LOCAL,true);

  f_list=new Fact(this,"list",tr("Vehicles list"),"",SectionItem,ConstData);
  bind(f_list);

  //JS register mandala
  parent->jsSync(this);
  foreach(QString key,f_local->f_mandala->constants.keys())
    parent->engine()->globalObject().setProperty(key,parent->engine()->toScriptValue(f_local->f_mandala->constants.value(key)));
  foreach (VehicleMandalaFact *f, f_local->f_mandala->allFacts()) {
    parent->jsexec(QString("this.__defineGetter__('%1', function(){ return app.vehicles.current.mandala.%1.value; });").arg(f->name()));
    parent->jsexec(QString("this.__defineSetter__('%1', function(v){ app.vehicles.current.mandala.%1.value=v; });").arg(f->name()));
  }


  selectVehicle(f_local);
  //parent->jsexec("var m=app.vehicles.current.mandala");

  //ident request timer
  reqTimer.setInterval(1000);
  connect(&reqTimer,&QTimer::timeout,[=](){
    if(reqList.isEmpty())reqTimer.stop();
    else emit sendUplink(reqList.takeFirst());
  });

  qmlRegisterUncreatableType<Vehicles>("GCS.Vehicles", 1, 0, "Vehicles", "Reference only");
  qmlRegisterUncreatableType<Vehicle>("GCS.Vehicles", 1, 0, "Vehicle", "Reference only");
  qmlRegisterUncreatableType<VehicleWarnings>("GCS.Vehicles", 1, 0, "VehicleWarnings", "Reference only");
}
//=============================================================================
void Vehicles::downlinkReceived(const QByteArray &ba)
{
  //qDebug()<<"Vehicles::downlinkReceived"<<ba.size();
  _bus_packet *packet=(_bus_packet*)ba.data();
  uint data_cnt=ba.size();
  if(data_cnt<=bus_packet_size_hdr)return;
  data_cnt-=bus_packet_size_hdr;
  switch(packet->id){
    case idx_xpdr:{      //transponder from UAV received
      if(data_cnt!=sizeof(IDENT::_xpdr))return;
      IDENT::_xpdr *xpdr=(IDENT::_xpdr*)packet->data;
      //qDebug()<<"xpdr"<<xpdr->squawk;
      //find uav in list by squawk
      Vehicle *v=squawkMap.value(xpdr->squawk);
      if(v){
        //unpack XPDR
        v->xpdrReceived(ba);
        break;
      }
      //new transponder detected, request IDENT
      reqIDENT(xpdr->squawk);
    }break;
    case idx_ident:{
      if(data_cnt!=sizeof(IDENT::_ident))return;
      IDENT::_ident *ident=(IDENT::_ident*)packet->data;
      //unpack ident
      ident->callsign[sizeof(ident->callsign)-1]=0;
      QString callsign((const char*)(ident->callsign));
      QByteArray uid((const char*)ident->uid,sizeof(ident->uid));
      quint16 squawk=ident->squawk;
      Vehicle::VehicleClass vclass=(Vehicle::VehicleClass)ident->vclass;

      //qDebug()<<"ident"<<callsign<<squawk;
      if((!squawk) || callsign.isEmpty()){
        //received zero SQUAWK
        assignIDENT(callsign,uid);
        break;
      }
      //find uav in list by uid
      Vehicle *v=NULL;
      foreach(FactTree *i,f_list->childItemsTree()){
        Vehicle *f=static_cast<Vehicle*>(i);
        if(f->f_uid->value().toByteArray().toHex()==uid.toHex()){
          v=f;
          break;
        }
      }
      if(v){
        //update from ident
        v->f_squawk->setValue(squawk);
        v->f_callsign->setValue(callsign);
        v->f_vclass->setValue(vclass);
      }else{
        //new Vehicle found
        v=new Vehicle(this,callsign,squawk,uid,vclass,false);
        qDebug("%s: %s '%s' (%.4X)",tr("Vehicle identified").toUtf8().data(),v->f_vclass->text().toUtf8().data(),callsign.toUtf8().data(),squawk);
        emit vehicleRegistered(v);
      }
      //check squawk with uid
      if(squawkMap.contains(squawk)){
        if(squawkMap.value(squawk)!=v){
          //duplicate squawk came with this ident
          qDebug("%s: %s (%.4X)",tr("Assigning squawk to").toUtf8().data(),callsign.toUtf8().data(),squawk);
          assignIDENT(callsign,uid);
          break;
        }
      }else{
        squawkMap.insert(squawk,v);
        if(squawkMap.size()==1) selectVehicle(v);
      }
    }break;
    case idx_dlink:{
      if(data_cnt<=sizeof(IDENT::_squawk))return;
      IDENT::_squawk squawk=packet->data[0]|packet->data[1]<<8;
      //find uav in list by squawk
      QByteArray pdata(ba.right(data_cnt-sizeof(IDENT::_squawk)));
      packet=(_bus_packet*)pdata.data();
      if(!squawk)break;
      //check if new transponder detected, request IDENT
      Vehicle *v=squawkMap.value(squawk);
      if(v) v->downlinkReceived(pdata);
      else reqIDENT(squawk);
    }break;
    default:
      f_local->downlinkReceived(ba);
  }
}
//=============================================================================
void Vehicles::vehicleSendUplink(Vehicle *v, const QByteArray &packet)
{
  //qDebug()<<"VS"<<v->title()<<v->squawk()<<packet.toHex().toUpper();
  if(v==f_local){
    emit sendUplink(packet);
    return;
  }
  //prepend idx_dlink+squawk
  emit sendUplink(QByteArray().append((unsigned char)idx_dlink).append((unsigned char)v->squawk()).append((unsigned char)(v->squawk()>>8)).append(packet));
}
//=============================================================================
//=============================================================================
void Vehicles::scheduleRequest(const QByteArray &ba)
{
  if(!reqList.contains(ba)){
    reqList.append(ba);
    reqTimer.start();
  }
}
//=============================================================================
void Vehicles::reqIDENT(quint16 squawk)
{
  scheduleRequest(QByteArray().append((unsigned char)idx_ident).append((unsigned char)squawk).append((unsigned char)(squawk>>8)));
}
//=============================================================================
void Vehicles::assignIDENT(QString callsign, QByteArray uid)
{
  IDENT::_ident ident;
  memset(&ident,0,sizeof(ident));
  //fix ident
  uint tcnt=1000000;
  while(tcnt--){
    ident.squawk=(qrand()+tcnt)^ident.squawk;
    if(ident.squawk<100||ident.squawk>0xff00)continue;
    if(squawkMap.contains(ident.squawk))continue;
    break;
  }
  if(!tcnt){
    qWarning("%s",tr("Can't find new squawk for assignment").toUtf8().data());
    return;
  }
  //unique squawk assigned, update callsign
  if(callsign.isEmpty()) callsign=QString("UAVOS-%1").arg((ulong)ident.squawk,4,16,QLatin1Char('0')).toUpper();
  callsign.truncate(sizeof(ident.callsign)-1);
  callsign=callsign.toUpper();
  memcpy(ident.callsign,callsign.toUtf8().data(),callsign.size());
  memcpy(ident.uid,uid.data(),sizeof(ident.uid));

  //send new ident
  QByteArray ba=QByteArray().append((unsigned char)idx_ident);
  int sz=ba.size();
  ba.resize(sz+sizeof(IDENT::_ident));
  memcpy(ba.data()+sz,&ident,sizeof(IDENT::_ident));
  emit sendUplink(ba);
}
//=============================================================================
//=============================================================================
void Vehicles::selectVehicle(Vehicle *v)
{
  qDebug("%s: %s '%s' (%s)",tr("Vehicle selected").toUtf8().data(),v->f_vclass->text().toUtf8().data(),v->f_callsign->text().toUtf8().data(),v->f_squawk->text().toUtf8().data());
  m_current=v;
  //update JSengine
  QQmlEngine *e=FactSystem::instance()->engine();
  //for QML
  e->rootContext()->setContextProperty("m",v->f_mandala);
  //for console
  //e->globalObject().setProperty("m",e->newQObject(v->f_mandala));

  //current vehicle signals wrappers
  foreach(QMetaObject::Connection c,currentVehicleConnections) disconnect(c);
  currentVehicleConnections.clear();
  currentVehicleConnections.append(connect(v->f_mandala,&VehicleMandala::dataReceived,this,&Vehicles::currentDataReceived));
  currentVehicleConnections.append(connect(v->f_mandala,&VehicleMandala::serialReceived,this,&Vehicles::currentSerialReceived));

  emit currentChanged();
  emit vehicleSelected(v);
  f_select->setStatus(v->title());
}
//=============================================================================
//=============================================================================
Vehicle * Vehicles::current(void) const
{
  return m_current;
}
//=============================================================================
//=============================================================================

