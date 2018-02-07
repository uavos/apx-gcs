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
  mission(parent),
  format(1)
{
  if(xmlMap.isEmpty()){
    xmlMap["order"]     ="";
    xmlMap["type"]      ="turn";
    xmlMap["hmsl"]      ="HMSL";
    xmlMap["poi"]       ="POI";
    xmlMap["radius"]    ="turnR";
    xmlMap["timeout"]   ="timeout";
  }
}
//=============================================================================
//=============================================================================
QDomDocument MissionXml::write() const
{
  QDomDocument doc;
  doc.appendChild(doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\""));
  write(doc);
  return doc;
}
//=============================================================================
void MissionXml::write(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("mission"));
  dom.toElement().setAttribute("fmt",QString::number(format));
  dom.toElement().setAttribute("href","http://www.uavos.com/");
  dom.toElement().setAttribute("title",mission->f_missionTitle->text());
  dom.toElement().setAttribute("version",FactSystem::version());
  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(QString(mission->hash().toHex().toUpper())));
  dom.appendChild(doc.createElement("date")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));
  //bounds
  QDomNode e=dom.appendChild(doc.createElement("bounds"));
  QGeoRectangle rect=mission->boundingGeoRectangle();
  QDomNode e2=e.appendChild(doc.createElement("topLeft"));
  e2.appendChild(doc.createElement("latitude")).appendChild(doc.createTextNode(QString("%1").arg(rect.topLeft().latitude(),0,'f')));
  e2.appendChild(doc.createElement("longitude")).appendChild(doc.createTextNode(QString("%1").arg(rect.topLeft().longitude(),0,'f')));
  e2=e.appendChild(doc.createElement("bottomRight"));
  e2.appendChild(doc.createElement("latitude")).appendChild(doc.createTextNode(QString("%1").arg(rect.bottomRight().latitude(),0,'f')));
  e2.appendChild(doc.createElement("longitude")).appendChild(doc.createTextNode(QString("%1").arg(rect.bottomRight().longitude(),0,'f')));
  //objects
  write(dom,mission->f_runways,"runways","runway");
  write(dom,mission->f_waypoints,"waypoints","waypoint");
  write(dom,mission->f_taxiways,"taxiways","taxiway");
  write(dom,mission->f_pois,"points","point");
}
//=============================================================================
void MissionXml::write(QDomNode dom, MissionGroup *group, const QString &sectionName, const QString &elementName) const
{
  if(group->size()<=0)return;
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement(sectionName));
  dom.toElement().setAttribute("cnt",QString::number(group->size()));
  for(int i=0;i<group->size();++i){
    QDomNode e=dom.appendChild(doc.createElement(elementName));
    e.toElement().setAttribute("id",QString::number(i));
    write(e,group->childFact(i));
  }
}
//=============================================================================
void MissionXml::write(QDomNode dom, Fact *fact) const
{
  QDomDocument doc=dom.ownerDocument();
  if(fact->size()<=0){
    QString s,su=fact->units();
    if(su=="lat"||su=="lon"||su=="time")s=fact->value().toString();
    else s=fact->text();
    dom.appendChild(doc.createTextNode(s));
    return;
  }
  //recursively save tree
  for(int i=0;i<fact->size();++i){
    Fact *f=fact->childFact(i);
    QString tag=xmlMap.value(f->name(),"null");
    if(tag.isEmpty())continue; //filtered field (order)
    write(dom.appendChild(doc.createElement(f->name())),f);
  }
}
//=============================================================================
//=============================================================================
bool MissionXml::read(QDomNode dom) const
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

    //read title
    mission->f_missionTitle->setValue(dom.toElement().attribute("title"));

    //read home pos
    /*QGeoCoordinate homeCoordinate;
    QDomElement e=dom.firstChildElement("home");
    if(!e.isNull()){
      homeCoordinate.setLatitude(e.firstChildElement("lat").text().toDouble());
      homeCoordinate.setLongitude(e.firstChildElement("lon").text().toDouble());
      homeCoordinate.setAltitude(e.firstChildElement("hmsl").text().toDouble());
    }*/

    return true;
  }
  qWarning("%s",QString(tr("The mission XML data format is not correct.")).toUtf8().data());
  return false;
}
//=============================================================================
int MissionXml::read(QDomNode dom, MissionGroup *group, const QString &sectionName, const QString &elementName) const
{
  //qDebug()<<group<<sectionName<<elementName;
  int rcnt=0;
  QDomElement e=dom.firstChildElement(sectionName);
  if(!e.isNull()){
    int cnt=e.attribute("cnt").toInt();
    for(e=e.firstChildElement(elementName);!e.isNull();e=e.nextSiblingElement(e.tagName())){
      int idx=e.hasAttribute("id")?e.attribute("id").toInt():e.attribute("idx").toInt();
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
int MissionXml::read(QDomNode dom, Fact *fact) const
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
