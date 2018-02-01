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
#include "MissionListModel.h"
#include "MissionTools.h"
#include "MissionGroup.h"
#include "MissionField.h"
#include "Waypoint.h"
#include "Runway.h"
#include "Taxiway.h"
#include "Poi.h"

#include <AppDirs.h>
#include <QFileDialog>

//pack
#include <Mandala.h>
#include <Mission.h>
#include <node.h>
//=============================================================================
VehicleMission::VehicleMission(Vehicle *parent)
  : Fact(parent,"mission","Mission",tr("Vehicle mission"),GroupItem,NoData),
    vehicle(parent),
    m_startHeading(0),
    m_startLength(0),
    m_empty(true),
    m_missionSize(0)
{
  setIconSource("ship-wheel");

  //actions
  f_request=new Fact(this,"request",tr("Request"),tr("Download from vehicle"),FactItem,ActionData);
  f_request->setValue(ButtonAction);
  f_request->setIconSource("download");
  connect(f_request,&Fact::triggered,vehicle,&Vehicle::requestMission);

  f_upload=new Fact(this,"upload",tr("Upload"),tr("Upload to vehicle"),FactItem,ActionData);
  f_upload->setEnabled(false);
  f_upload->setValue(ApplyAction);
  f_upload->setIconSource("upload");
  connect(f_upload,&Fact::triggered,this,&VehicleMission::uploadMission);
  connect(f_upload,&Fact::enabledChanged,this,&VehicleMission::actionsUpdated);

  f_clear=new Fact(this,"clear",tr("Clear"),tr("Clear mission"),FactItem,ActionData);
  f_clear->setEnabled(false);
  f_clear->setValue(RemoveAction);
  connect(f_clear,&Fact::triggered,this,&VehicleMission::clearMission);
  connect(f_clear,&Fact::enabledChanged,this,&VehicleMission::actionsUpdated);

  f_export=new Fact(this,"export",tr("Save"),tr("Export mission"),FactItem,ActionData);
  f_export->setValue(ButtonAction);
  f_export->setIconSource("content-save");
  connect(f_export,&Fact::triggered,this,&VehicleMission::save);

  f_import=new Fact(this,"import",tr("Load"),tr("Import mission"),FactItem,ActionData);
  f_import->setValue(ButtonAction);
  f_import->setIconSource("folder-open");
  connect(f_import,&Fact::triggered,this,&VehicleMission::load);


  f_missionTitle=new MissionField(this,"mtitle",tr("Title"),tr("Mission title"),TextData);


  connect(this,&VehicleMission::emptyChanged,this,&VehicleMission::updateActions);


  //groups of items
  f_runways=new Runways(this,"runways",tr("Runways"),tr("Takeoff and Landing"));
  f_waypoints=new Waypoints(this,"waypoints",tr("Waypoints"),"");
  f_taxiways=new Taxiways(this,"taxiways",tr("Taxiways"),"");
  f_pois=new Pois(this,"points",tr("Points"),tr("Points of Interest"));

  foreach (MissionGroup *group, groups) {
    connect(group,&Fact::sizeChanged,this,&VehicleMission::updateSize,Qt::QueuedConnection);
  }

  //tools
  f_tools=new MissionTools(this);

  //internal
  m_listModel=new MissionListModel(this);

  connect(this,&Fact::modifiedChanged,this,&VehicleMission::updateActions);


  connect(this,&VehicleMission::startPointChanged,this,&VehicleMission::updateStartPath);
  connect(this,&VehicleMission::startHeadingChanged,this,&VehicleMission::updateStartPath);
  connect(this,&VehicleMission::startLengthChanged,this,&VehicleMission::updateStartPath);

  updateActions();

  if(!vehicle->isLocal()){
    //f_request->trigger();
    QTimer::singleShot(2000,f_request,&Fact::trigger);
  }

  qmlRegisterUncreatableType<VehicleMission>  ("GCS.Mission", 1, 0, "Mission", "Reference only");
  qmlRegisterUncreatableType<MissionItem>     ("GCS.Mission", 1, 0, "MissionItem", "Reference only");
  qmlRegisterUncreatableType<Waypoint>        ("GCS.Mission", 1, 0, "Waypoint", "Reference only");
  qmlRegisterUncreatableType<Runway>          ("GCS.Mission", 1, 0, "Runway", "Reference only");
  qmlRegisterUncreatableType<MissionListModel>("GCS.Mission", 1, 0, "MissionListModel", "Reference only");

  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void VehicleMission::updateActions()
{
  bool bEmpty=empty();
  f_upload->setEnabled(!bEmpty);
  f_clear->setEnabled(!bEmpty);
  /*bool busy=false;//model->requestManager.busy();
  bool upgrading=false;//model->isUpgrading();
  bool bModAll=modified();
  bool bEmpty=nodesCount()<=0;
  f_upload->setEnabled(bModAll && (!(busy)));
  f_stop->setEnabled(busy||upgrading);
  f_reload->setEnabled(!(upgrading||bEmpty));*/
}
void VehicleMission::updateSize()
{
  int cnt=0;
  foreach (MissionGroup *group, groups) {
    cnt+=group->size();
  }
  setMissionSize(cnt);
  setEmpty(cnt<=0);
  setStatus(cnt>0?QString::number(cnt):"");
}
//=============================================================================
void VehicleMission::updateStartPath()
{
  if(f_waypoints->size()<=0)return;
  static_cast<Waypoint*>(f_waypoints->child(0))->updatePath();
}
//=============================================================================
QGeoRectangle VehicleMission::boundingGeoRectangle() const
{
  QList<QGeoCoordinate> clist;
  foreach (MissionGroup *group, groups) {
    for(int i=0;i<group->size();++i){
      clist.append(static_cast<MissionItem*>(group->child(i))->coordinate());
    }
  }
  for(int i=0;i<f_runways->size();++i){
    Runway *e=static_cast<Runway*>(f_runways->child(i));
    QGeoCoordinate p(e->coordinate());
    clist.append(p.atDistanceAndAzimuth(e->f_approach->value().toDouble()*1.2,e->heading()+180.0));
  }
  QGeoRectangle r(clist);
  r.setWidth(r.width()*1.2);
  r.setHeight(r.height()*1.2);
  return r;
}
//=============================================================================
//=============================================================================
//=============================================================================
void VehicleMission::clearMission()
{
  foreach (MissionGroup *group, groups) {
    group->f_clear->trigger();
  }
  setModified(false,true);
  /*if(snMap.isEmpty())return;
  snMap.clear();
  nGroups.clear();
  f_list->removeAll();
  setModified(false);
  f_list->setModified(false);*/
  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void VehicleMission::uploadMission()
{
  test();
}
//=============================================================================
void VehicleMission::test(int n)
{
  if(f_waypoints->size()<=0)return;
  Waypoint *w=static_cast<Waypoint*>(f_waypoints->childItems().last());
  QGeoCoordinate p(w->f_latitude->value().toDouble(),w->f_longitude->value().toDouble());
  double hdg=360.0*qrand()/RAND_MAX;
  for(int i=0;i<n;++i){
    hdg+=200.0*qrand()/RAND_MAX-100.0;
    p=p.atDistanceAndAzimuth(100+10000.0*qrand()/RAND_MAX,hdg);
    f_waypoints->addObject(p);
  }
}
//=============================================================================
bool VehicleMission::unpackMission(const QByteArray &ba)
{
  _bus_packet &packet=*(_bus_packet*)ba.data();
  uint data_cnt=ba.size();
  if(data_cnt<bus_packet_size_hdr)return false;
  data_cnt-=bus_packet_size_hdr;
  if(data_cnt<4)return false;
  if(packet.id!=idx_mission) return false;
  int ecnt=0,wpcnt=0,rwcnt=0;
  const uint8_t *ptr=packet.data;
  for(uint cnt=0;data_cnt>=sizeof(Mission::_item_hdr);data_cnt-=cnt){
    ptr+=cnt;
    ecnt++;
    const Mission::_item_hdr *hdr=(Mission::_item_hdr*)ptr;
    switch(hdr->type){
      case Mission::mi_stop:
        data_cnt-=sizeof(Mission::_item_hdr);
        ecnt--;
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
        Poi *f=new Poi(f_pois);
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
    setEmpty(true);
  }else if(ecnt<=0){
    setEmpty(true);
    qDebug()<<"Mission empty";
  }else{
    setEmpty(false);
    backup();
    qDebug()<<"Mission received"<<ecnt;
  }
  setModified(false,true);

  return true;
}
//=============================================================================
void VehicleMission::backup()
{
  foreach (MissionGroup *group, groups) {
    group->backup();
  }
  f_missionTitle->backup();
  setModified(false,true);
}
void VehicleMission::restore()
{
  foreach (MissionGroup *group, groups) {
    group->restore();
  }
  setModified(false,true);
}
//=============================================================================
void VehicleMission::save() const
{
  if(!AppDirs::missions().exists()) AppDirs::missions().mkpath(".");
  QFileDialog dlg(NULL,f_export->descr(),AppDirs::missions().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  QStringList filters;
  filters << tr("Mission files")+" (*.xml *.mission)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  dlg.setDefaultSuffix("mission");
  QString fname=f_missionTitle->text().replace(' ','-');
  if(!fname.isEmpty())fname.append("-");
  fname.append(vehicle->f_callsign->text());
  dlg.selectFile(AppDirs::configs().filePath(fname));
  if(!dlg.exec() || dlg.selectedFiles().size()!=1)return;

  fname=dlg.selectedFiles().first();
  QFile file(fname);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QTextStream stream(&file);
  //vehicle->f_nodes->xml->write().save(stream,2);
  file.close();
}
//=============================================================================
void VehicleMission::load()
{
}
//=============================================================================
//=============================================================================
//=============================================================================
QGeoCoordinate VehicleMission::mapCoordinate() const
{
  return m_mapCoordinate;
}
void VehicleMission::setMapCoordinate(const QGeoCoordinate &v)
{
  if(m_mapCoordinate==v)return;
  m_mapCoordinate=v;
  emit mapCoordinateChanged();
}
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
MissionListModel * VehicleMission::listModel() const
{
  return m_listModel;
}
bool VehicleMission::empty() const
{
  return m_empty;
}
void VehicleMission::setEmpty(const bool v)
{
  if(m_empty==v)return;
  m_empty=v;
  emit emptyChanged();
}
int VehicleMission::missionSize() const
{
  return m_missionSize;
}
void VehicleMission::setMissionSize(const int v)
{
  if(m_missionSize==v)return;
  m_missionSize=v;
  emit missionSizeChanged();
}
//=============================================================================
//=============================================================================
