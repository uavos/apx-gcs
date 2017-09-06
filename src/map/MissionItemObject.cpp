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
#include "MissionItemObject.h"
#include "MissionModel.h"
//=============================================================================
MissionItemObject::MissionItemObject(MissionItem *parent,MissionModel *model,QString childName)
  : MissionItem(parent,childName),
    mapItem(NULL),model(model)
{
  connect(model->selectionModel,SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this,SLOT(modelSelectionChanged(QItemSelection,QItemSelection)));
  connect(this,SIGNAL(changed()),model,SLOT(emit_layoutChanged()),Qt::QueuedConnection);
}
MissionItemObject::~MissionItemObject()
{
  if(mapItem && model->mapView->scene()){
    model->mapView->scene()->removeItem(mapItem);
    mapItem->deleteLater();
  }
}
//=============================================================================
void MissionItemObject::modelSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
  if(!mapItem)return;
  emit changed();
  //select on map
  QModelIndex index=model->findIndex(this);//index(row(),0,model->index(parentItem->row(),0));
  if(deselected.contains(index) && mapItem->isSelected()){
    mapItem->setSelected(false);
  }
  if(selected.contains(index) && mapItem->isSelected()==false){
    mapItem->setSelected(true);
  }
  //emit changed();
}
//=============================================================================
void MissionItemObject::mapItemSelected(bool v)
{
  QModelIndex index=model->findIndex(this);//row(),0,model->index(parentItem->row(),0));
  if(model->selectionModel->isSelected(index)==v)return;
  model->selectionModel->select(index,(v?QItemSelectionModel::Select:QItemSelectionModel::Deselect)|QItemSelectionModel::Rows);
}
//=============================================================================
Qt::ItemFlags MissionItemObject::flags(int column) const
{
  return MissionItem::flags(column)|Qt::ItemIsDragEnabled;
}
//=============================================================================
void MissionItemObject::saveToXml(QDomNode dom) const
{
  if(!childCount())return;
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement(objectName()));
  dom.toElement().setAttribute("idx",row());
  MissionItem::saveToXml(dom);
}
//=============================================================================
void MissionItemObject::loadFromXml(QDomNode dom)
{
  //load fields
  foreach(MissionItem *i,childItems)
    i->loadFromXml(dom);
}
//=============================================================================
//=============================================================================
