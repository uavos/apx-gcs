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
#include "VehicleMission.h"
#include "Vehicle.h"
#include "MissionItems.h"
#include "Waypoints.h"
#include "Waypoint.h"
#include "Runways.h"
#include "Runway.h"
#include "Taxiways.h"
#include "Taxiway.h"
#include "Points.h"
#include "Point.h"

//pack
namespace APX{
#include <Mandala.h>
}
#include <Mission.h>
#include <node.h>
//=============================================================================
VehicleMission::VehicleMission(Vehicle *parent)
  : Fact(parent,"mission","Mission",tr("Vehicle mission"),GroupItem,NoData),
    vehicle(parent),
    m_startHeading(0),
    m_startLength(0)
{
  f_request=new Fact(this,"request",tr("Request"),tr("Download from vehicle"),FactItem,ActionData);
  connect(f_request,&Fact::triggered,vehicle,&Vehicle::requestMission);

  f_upload=new Fact(this,"upload",tr("Upload"),tr("Upload to vehicle"),FactItem,ActionData);
  connect(f_upload,&Fact::triggered,this,&VehicleMission::upload);
  connect(f_upload,&Fact::enabledChanged,this,&VehicleMission::actionsUpdated);

  f_stop=new Fact(this,"stop",tr("Stop"),tr("Stop data requests"),FactItem,ActionData);
  connect(f_stop,&Fact::triggered,this,&VehicleMission::stop);
  connect(f_stop,&Fact::enabledChanged,this,&VehicleMission::actionsUpdated);

  f_runways=new Runways(this);
  f_waypoints=new Waypoints(this);
  f_taxiways=new Taxiways(this);
  f_points=new Points(this);

  connect(this,&Fact::modifiedChanged,this,&VehicleMission::updateActions);


  connect(this,&VehicleMission::startPointChanged,this,&VehicleMission::updateStartPath);
  connect(this,&VehicleMission::startHeadingChanged,this,&VehicleMission::updateStartPath);
  connect(this,&VehicleMission::startLengthChanged,this,&VehicleMission::updateStartPath);

  updateActions();

  if(!vehicle->isLocal()){
    //f_request->trigger();
    QTimer::singleShot(2000,f_request,&Fact::trigger);
  }

  qmlRegisterUncreatableType<Waypoint>("GCS.Mission", 1, 0, "Waypoint", "Reference only");
  qmlRegisterUncreatableType<Runway>("GCS.Mission", 1, 0, "Runway", "Reference only");

  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void VehicleMission::updateActions()
{
  /*bool busy=false;//model->requestManager.busy();
  bool upgrading=false;//model->isUpgrading();
  bool bModAll=modified();
  bool bEmpty=nodesCount()<=0;
  f_upload->setEnabled(bModAll && (!(busy)));
  f_stop->setEnabled(busy||upgrading);
  f_reload->setEnabled(!(upgrading||bEmpty));*/
}
//=============================================================================
void VehicleMission::updateStartPath()
{
  if(f_waypoints->size()<=0)return;
  static_cast<Waypoint*>(f_waypoints->child(0))->updatePath();
}
//=============================================================================
//=============================================================================
void VehicleMission::clear()
{
  /*if(snMap.isEmpty())return;
  snMap.clear();
  nGroups.clear();
  f_list->removeAll();
  setModified(false);
  f_list->setModified(false);*/
  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void VehicleMission::upload()
{
  /*foreach(NodeItem *node,snMap.values()){
    node->upload();
  }*/
}
//=============================================================================
void VehicleMission::stop()
{
}
//=============================================================================
bool VehicleMission::unpackMission(const QByteArray &ba)
{
  _bus_packet &packet=*(_bus_packet*)ba.data();
  uint data_cnt=ba.size();
  if(data_cnt<bus_packet_size_hdr)return false;
  data_cnt-=bus_packet_size_hdr;
  if(data_cnt<4)return false;
  if(packet.id!=APX::idx_mission) return false;
  int wpcnt=0,rwcnt=0;
  const uint8_t *ptr=packet.data;
  for(uint cnt=0;data_cnt>=sizeof(Mission::_item_hdr);data_cnt-=cnt){
    ptr+=cnt;
    const Mission::_item_hdr *hdr=(Mission::_item_hdr*)ptr;
    switch(hdr->type){
      case Mission::mi_stop:
        data_cnt-=sizeof(Mission::_item_hdr);
      break;
      case Mission::mi_wp:
      {
        cnt=sizeof(Mission::_item_wp);
        if(data_cnt<cnt)break;
        wpcnt++;
        const Mission::_item_wp *e=(Mission::_item_wp*)ptr;
        Waypoint *f=new Waypoint(f_waypoints);
        f->f_altitude->setValue(e->alt);
        f->f_type->setValue(e->hdr.option);
        f->f_latitude->setValue(e->lat);
        f->f_longitude->setValue(e->lon);
        bool err=false;
        while((data_cnt-cnt)>=(int)sizeof(Mission::_item_hdr) && ((Mission::_item_hdr*)(ptr+cnt))->type==Mission::mi_action){
          Mission::_item_action *v=(Mission::_item_action*)(ptr+cnt);
          uint sz=Mission::action_size(v->hdr.option);
          if((data_cnt-cnt)<sz){
            err=true;
            break; //error
          }
          cnt+=sz;
          switch(v->hdr.option){
            case Mission::mo_speed:   f->f_speed->setValue(v->speed);break;
            case Mission::mo_poi:     f->f_poi->setValue(v->poi+1);break;
            case Mission::mo_scr:     f->f_script->setValue(QString(v->scr));break;
            case Mission::mo_loiter:
              f->f_loiter->setValue(1);
              f->f_turnR->setValue(v->loiter.turnR);
              f->f_loops->setValue(v->loiter.loops);
              f->f_time->setValue(v->loiter.timeS);
            break;
            case Mission::mo_shot:{
              switch(v->shot.opt){
                case 0: //single
                  f->f_shot->setValue(1);
                  f->f_dshot->setValue(0);
                break;
                case 1: //start
                  f->f_shot->setValue(2);
                  f->f_dshot->setValue(v->shot.dist);
                break;
                case 2: //stop
                  f->f_shot->setValue(2);
                  f->f_dshot->setValue(0);
                break;
              }
            }break;
            default:
              err=true;
            break;
          }
        }
        if(err)break;
      }
      continue;
      case Mission::mi_rw:
      {
        const Mission::_item_rw *e=(Mission::_item_rw*)ptr;
        cnt=sizeof(Mission::_item_rw);
        if(data_cnt<cnt)break;
        rwcnt++;
        Runway *f=new Runway(f_runways);
        f->f_hmsl->setValue(e->hmsl);
        f->f_approach->setValue(e->approach);
        f->f_type->setValue(e->hdr.option);
        f->f_latitude->setValue(e->lat);
        f->f_longitude->setValue(e->lon);
        f->f_dN->setValue(e->dN);
        f->f_dE->setValue(e->dE);
      }
      continue;
      case Mission::mi_tw:
      {
        const Mission::_item_tw *e=(Mission::_item_tw*)ptr;
        cnt=sizeof(Mission::_item_tw);
        if(data_cnt<cnt)break;
        Taxiway *f=new Taxiway(f_taxiways);
        f->f_latitude->setValue(e->lat);
        f->f_longitude->setValue(e->lon);
      }
      continue;
      case Mission::mi_pi:
      {
        const Mission::_item_pi *e=(Mission::_item_pi*)ptr;
        cnt=sizeof(Mission::_item_pi);
        if(data_cnt<cnt)break;
        Point *f=new Point(f_points);
        f->f_latitude->setValue(e->lat);
        f->f_longitude->setValue(e->lon);
        f->f_hmsl->setValue(e->hmsl);
        f->f_radius->setValue(e->turnR);
        f->f_loops->setValue(e->loops);
        f->f_time->setValue(e->timeS);
      }
      continue;
      case Mission::mi_action:
      {
        const Mission::_item_action *e=(Mission::_item_action*)ptr;
        cnt=Mission::action_size(e->hdr.option);
        if(data_cnt<cnt)break;
      }
      continue;
      case Mission::mi_restricted:
      {
        const Mission::_item_area *e=(Mission::_item_area*)ptr;
        cnt=Mission::area_size(e->pointsCnt);
        if(data_cnt<cnt)break;
      }
      continue;
      case Mission::mi_emergency:
      {
        const Mission::_item_area *e=(Mission::_item_area*)ptr;
        cnt=Mission::area_size(e->pointsCnt);
        if(data_cnt<cnt)break;
      }
      continue;
    }
    break;
  }
  if(data_cnt){
    qWarning()<<"error in mission";
  }else{
    qDebug()<<"Mission received"<<wpcnt<<rwcnt;
  }

  return true;
}
//=============================================================================
//=============================================================================
QGeoCoordinate VehicleMission::startPoint() const
{
  return m_startPoint;
}
void VehicleMission::setStartPoint(const QGeoCoordinate &v)
{
  if(m_startPoint==v)return;
  m_startPoint=v;
  emit startPointChanged();
}
double VehicleMission::startHeading() const
{
  return m_startHeading;
}
void VehicleMission::setStartHeading(const double &v)
{
  if(m_startHeading==v)return;
  m_startHeading=v;
  emit startHeadingChanged();
}
double VehicleMission::startLength() const
{
  return m_startLength;
}
void VehicleMission::setStartLength(const double &v)
{
  if(m_startLength==v)return;
  m_startLength=v;
  emit startLengthChanged();
}
//=============================================================================
//=============================================================================
