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
#include "MissionXml.h"
#include <FactSystem.h>
#include <AppDirs.h>
#include <QtPositioning>
#include "VehicleMission.h"
#include "MissionItem.h"
#include "MissionGroup.h"
//=============================================================================
QMap<QString,QString> MissionXml::xmlMap;
//=============================================================================
MissionXml::MissionXml(VehicleMission *parent)
  : QObject(parent),
  mission(parent)
{
  if(xmlMap.isEmpty()){
    xmlMap["order"]     ="";
    xmlMap["type"]      ="turn";
    xmlMap["hmsl"]      ="HMSL";
    xmlMap["wpactions"] ="actions";
    xmlMap["poi"]       ="POI";
    xmlMap["radius"]    ="turnR";
    xmlMap["timeout"]   ="timeout";
  }
}
//=============================================================================
bool MissionXml::read(QDomNode dom)
{
  int rcnt=0;
  while(dom.nodeName()=="mission"){
    int ecnt;
    ecnt=read(dom,mission->f_runways,"runways","runway");
    if(ecnt<0)break;
    rcnt+=ecnt;
    ecnt=read(dom,mission->f_waypoints,"waypoints","waypoint");
    if(ecnt<0)break;
    rcnt+=ecnt;
    ecnt=read(dom,mission->f_taxiways,"taxiways","taxiway");
    if(ecnt<0)break;
    rcnt+=ecnt;
    ecnt=read(dom,mission->f_pois,"points","point");
    if(ecnt<0)break;
    rcnt+=ecnt;
    if(rcnt<=0)break;

    //read home pos
    QGeoCoordinate homeCoordinate;
    QDomElement e=dom.firstChildElement("home");
    if(!e.isNull()){
      homeCoordinate.setLatitude(e.firstChildElement("lat").text().toDouble());
      homeCoordinate.setLongitude(e.firstChildElement("lon").text().toDouble());
      homeCoordinate.setAltitude(e.firstChildElement("hmsl").text().toDouble());
    }

    return true;
  }
  qWarning("%s",QString(tr("The mission XML data format is not correct.")).toUtf8().data());
  return false;
}
//=============================================================================
int MissionXml::read(QDomNode dom, MissionGroup *group, const QString &sectionName, const QString &elementName)
{
  //qDebug()<<group<<sectionName<<elementName;
  int rcnt=0;
  QDomElement e=dom.firstChildElement(sectionName);
  if(!e.isNull()){
    int cnt=e.attribute("cnt").toInt();
    for(e=e.firstChildElement(elementName);!e.isNull();e=e.nextSiblingElement(e.tagName())){
      int idx=e.attribute("idx").toInt();
      if(idx!=group->size()) return -1;
      MissionItem *item=group->createObject();
      if(read(e,item)<=0) return -1;
      rcnt++;
    }
    if(rcnt!=cnt)return -1;
  }
  return rcnt;
}
//=============================================================================
int MissionXml::read(QDomNode dom, Fact *fact)
{
  if(fact->size()<=0){
    fact->setValue(dom.toElement().text());
    return 1;
  }
  //recursively load tree
  int rcnt=0;
  for(int i=0;i<fact->size();++i){
    Fact *f=fact->childFact(i);
    QString tag=xmlMap.value(f->name(),"null");
    if(tag.isEmpty())continue; //filtered field (order)
    QDomElement e=dom.firstChildElement(f->name());
    if(e.isNull()){
      //try map tags
      e=dom.firstChildElement(tag);
      if(e.isNull())continue;
    }
    rcnt+=read(e,f);
  }
  return rcnt;
}
//=============================================================================
//=============================================================================
