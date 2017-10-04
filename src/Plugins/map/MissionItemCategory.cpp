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
#include "MissionItemCategory.h"
#include "MissionModel.h"
//=============================================================================
MissionItemCategoryBase::MissionItemCategoryBase(MissionModel *model, QString name, QString caption,QString childName,Mission::_item_type mi_type)
 : MissionItem(model->rootItem,name,caption),
   model(model),childName(childName),mi_type(mi_type)
{
  connect(model->selectionModel,SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this,SLOT(modelSelectionChanged(QItemSelection,QItemSelection)));
  connect(this,SIGNAL(changed()),model,SIGNAL(changed()));
  connect(this,SIGNAL(addedRemoved()),model,SIGNAL(addedRemoved()));
}
//=============================================================================
QList<MissionItemObject *> MissionItemCategoryBase::selectedObjects(void) const
{
  QList<MissionItemObject *> list;
  foreach(QModelIndex index,model->selectionModel->selectedIndexes()){
    if(!(index.internalPointer()&&index.column()==0))continue;
    MissionItem *i=static_cast<MissionItem*>(index.internalPointer());
    if(!i->childCount())continue;//skip fields i=i->parent();
    if(i->parent()!=this && i->parent()->parent()!=this)continue;
    MissionItemObject *obj=static_cast<MissionItemObject*>(i);
    if(!list.contains(obj)) list.append(obj);
  }
  //qDebug()<<caption()<<list.size();
  return list;
}
//=============================================================================
void MissionItemCategoryBase::modelSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
  Q_UNUSED(deselected)
  if(!childCount())return;
  QModelIndex index=model->findIndex(this);
  if(selected.contains(index)){
    QTimer::singleShot(100,this,SLOT(selectAll()));
  }
}
void MissionItemCategoryBase::selectAll()
{
  QModelIndex index=model->findIndex(this);
  QItemSelection sel(model->index(0,0,index),model->index(childCount()-1,0,index));
  model->selectionModel->select(sel,QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
}
//=============================================================================
QVariant MissionItemCategoryBase::value(void) const
{
  if(!childCount()) return QVariant();
  return "("+QString::number(childCount())+")";
}
//=============================================================================
Qt::ItemFlags MissionItemCategoryBase::flags(int column) const
{
  Qt::ItemFlags f=MissionItem::flags(column)|Qt::ItemIsDropEnabled;
  if(column==tc_value && childCount()) f|=Qt::ItemIsEditable;
  return f;
}
//=============================================================================
void MissionItemCategoryBase::saveToXml(QDomNode dom) const
{
  if(!childCount())return;
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement(objectName()));
  dom.toElement().setAttribute("cnt",childCount());
  MissionItem::saveToXml(dom);
}
//=============================================================================
void MissionItemCategoryBase::loadFromXml(QDomNode dom)
{
  QDomElement e=dom.firstChildElement(objectName()); //find category
  if(!e.isNull()){
    int cnt=e.attribute("cnt").toInt();
    e=e.firstChildElement(childName);
    for(int i=0;(!e.isNull())&&i<cnt;i++){
      int idx=e.attribute("idx").toInt();
      if(childCount()<=idx)add();
      childItems.at(i)->loadFromXml(e);
      e=e.nextSiblingElement(e.tagName());
    }
    return;
  }
  //old format (plain)
  e=dom.firstChildElement(childName);
  for(int i=0;!e.isNull();i++){
    if(childCount()<=i)add();
    childItems.at(i)->loadFromXml(e);
    e=e.nextSiblingElement(e.tagName());
  }
}
//=============================================================================
int MissionItemCategoryBase::unpack(const QByteArray &ba)
{
  if(ba.size()<(int)sizeof(Mission::_item_hdr))return 0;
  if(((Mission::_item_hdr*)ba.data())->type==Mission::mi_stop)return ba.size();
  if(((Mission::_item_hdr*)ba.data())->type!=mi_type)return 0;
  add();
  return childItems.at(childCount()-1)->unpack(ba);
}
//=============================================================================
