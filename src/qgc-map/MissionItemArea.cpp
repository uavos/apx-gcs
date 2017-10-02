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
#include "MissionModel.h"
#include "MissionItemArea.h"
#include "MissionItemField.h"
#include "QMandala.h"
#include "MissionItemAreaPoint.h"
//=============================================================================
MissionItemArea::MissionItemArea(MissionItemCategory<MissionItemArea> *parent)
 : MissionItemObject(parent,parent->model,parent->childName),
   mi_type(parent->mi_type),
   areaName(parent->caption()),
   distance(0)
{
  connect(model,SIGNAL(addedRemoved()),this,SLOT(updatePath()),Qt::QueuedConnection);

  //QColor color=parent->mi_type==Mission::mi_restricted?Qt::red:Qt::green;

  updatePath();

  childName="vertex";
  addPoint();
  addPoint();
  addPoint();
  addPoint();
}
//=============================================================================
QVariant MissionItemArea::data(int column,int role) const
{
  if(column!=tc_field || role!=Qt::DisplayRole) return MissionItem::data(column,role);
  QStringList st;
  st.append("area "+QString::number(row()+1));
  return st.join(' ');
}
//=============================================================================
QVariant MissionItemArea::value(void) const
{
  QStringList st;
  st.append(QMandala::distanceToString(distance));
  st.append(descr());
  st.removeAll("");
  return st.join(' ');
}
//=============================================================================
QStringList MissionItemArea::getToolTip(void) const
{
  QStringList st=MissionItem::getToolTip();
  QString s=st.first();
  st.clear();
  st.append(s);
  st.append(QString("%1: %2 m").arg("DH").arg((uint)distance));
  return st;
}
//=============================================================================
void MissionItemArea::updatePath()
{
  path.clear();
  distance=0;
  foreach(MissionItem *i,childItems){
    MissionItemAreaPoint *v=static_cast<MissionItemAreaPoint*>(i);
    QPointF p(v->f_latitude->value().toDouble(),v->f_longitude->value().toDouble());
    path.append(p);
    //calc closest point distance
    double dist=QMandala::instance()->current->distance(QMandala::instance()->current->lla2ne(Vect(p.x(),p.y(),0)));
    if(distance==0 || distance>dist)distance=dist;
  }
  emit pathChanged();
}
//=============================================================================
void MissionItemArea::addPointToSelectedObject()
{
  QList<MissionItemAreaPoint *> list=selectedPoints();
  if(list.size()!=1)return;
  MissionItemAreaPoint *i=addPoint();
  //move item after selected one
  childItems.removeOne(i);
  childItems.insert(childItems.indexOf(list.first())+1,i);
  updatePath();
}
//=============================================================================
MissionItemAreaPoint * MissionItemArea::addPoint()
{
  MissionItemAreaPoint *i=new MissionItemAreaPoint(this,childName);
  updatePath();
  connect(i,SIGNAL(pathChanged()),this,SLOT(updatePath()));
  //connect(i->mapItem,SIGNAL(selected(bool)),this,SLOT(mapItemSelected(bool)));
  return i;
}
//=============================================================================
QList<MissionItemAreaPoint *> MissionItemArea::selectedPoints(void) const
{
  QList<MissionItemAreaPoint *> list;
  foreach(MissionItem *i,childItems){
    MissionItemAreaPoint *v=static_cast<MissionItemAreaPoint*>(i);
    if(model->selectionModel->selectedIndexes().contains(model->findIndex(v)))
      list.append(v);
  }
  return list;
}
//=============================================================================
void MissionItemArea::mapItemSelected(bool v)
{
  //update selectionmodel
  Q_UNUSED(v)
  /*bool bSelArea=false;
  foreach(MissionItem *i,childItems){
    MissionItemAreaPoint *v=static_cast<MissionItemAreaPoint*>(i);
    bool bSel=v->mapItem->isSelected();
    model->selectionModel->select(model->findIndex(v),(bSel?QItemSelectionModel::Select:QItemSelectionModel::Deselect)|QItemSelectionModel::Rows);
    if(bSel) bSelArea=true;
  }
  bSelArea=mapItem->isSelected();
  model->selectionModel->select(model->findIndex(this),(bSelArea?QItemSelectionModel::Select:QItemSelectionModel::Deselect)|QItemSelectionModel::Rows);*/
}
//=============================================================================
void MissionItemArea::saveToXml(QDomNode dom) const
{
  if(!childCount())return;
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement(objectName()));
  dom.toElement().setAttribute("idx",row());
  dom.toElement().setAttribute("cnt",childCount());
  MissionItem::saveToXml(dom);
}
//=============================================================================
void MissionItemArea::loadFromXml(QDomNode dom)
{
  //load points
  clear();
  QDomElement e=dom.toElement();
  int cnt=e.attribute("cnt").toInt();
  e=e.firstChildElement(childName);
  for(int i=0;(!e.isNull())&&i<cnt;i++){
    int idx=e.attribute("idx").toInt();
    if(childCount()<=idx) addPoint();
    childItems.at(i)->loadFromXml(e);
    e=e.nextSiblingElement(e.tagName());
  }
}
//=============================================================================
QByteArray MissionItemArea::pack() const
{
  QByteArray ba;
  if(!childCount())return ba;
  Mission::_item_area v;
  v.hdr.type=mi_type;
  v.pointsCnt=childCount();
  ba.append(QByteArray((const char*)&v,sizeof(v)));
  foreach(MissionItem *i,childItems){
    Mission::_item_area_point vp;
    MissionItemAreaPoint *p=static_cast<MissionItemAreaPoint*>(i);
    vp.lat=p->f_latitude->value().toFloat();
    vp.lon=p->f_longitude->value().toFloat();
    ba.append(QByteArray((const char*)&vp,sizeof(vp)));
  }
  return ba;
}
int MissionItemArea::unpack(const QByteArray &ba)
{
  if(ba.size()<(int)sizeof(Mission::_item_area))return 0;
  Mission::_item_area *v=(Mission::_item_area*)ba.data();
  if(v->hdr.type!=mi_type)return 0;
  clear();
  int cnt=sizeof(Mission::_item_area);
  int pcnt=v->pointsCnt;
  for(int i=0;i<pcnt;i++){
    if(cnt>=ba.size())return 0;
    if(childCount()<=i) addPoint();
    Mission::_item_area_point *vp=(Mission::_item_area_point*)(ba.data()+cnt);
    MissionItemAreaPoint *p=static_cast<MissionItemAreaPoint*>(childItems.at(i));
    p->f_latitude->setValue(vp->lat);
    p->f_longitude->setValue(vp->lon);
    cnt+=sizeof(Mission::_item_area_point);
  }
  return cnt;
}
//=============================================================================

