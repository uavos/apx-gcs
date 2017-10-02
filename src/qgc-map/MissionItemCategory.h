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
#ifndef MissionItemCategory_H
#define MissionItemCategory_H
//=============================================================================
#include <QtQuick>
#include "MissionItem.h"
#include "Mission.h"
class MissionModel;
class MissionItemObject;
//=============================================================================
class MissionItemCategoryBase: public MissionItem
{
  Q_OBJECT
public:
  MissionItemCategoryBase(MissionModel *model,QString name,QString caption,QString childName,Mission::_item_type mi_type);
  QList<MissionItemObject *> selectedObjects(void) const;

  QVariant value(void) const;
  Qt::ItemFlags flags(int column) const;

  MissionModel *model;
  QString childName;
  Mission::_item_type mi_type; //binary packet data hdr byte
protected:
  void saveToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);

  int unpack(const QByteArray &ba);
protected slots:
  void modelSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
  void selectAll();
public slots:
  virtual void add()=0;

signals:
  void changed();
  void addedRemoved();
  void addPointToSelectedObject();
};
//=============================================================================
template <class T_map>
class MissionItemCategory: public MissionItemCategoryBase
{
public:
  MissionItemCategory(MissionModel *model,QString name,QString caption,QString childName,Mission::_item_type mi_type)
   : MissionItemCategoryBase(model,name,caption,childName,mi_type)
  {
  }
  T_map * createItem()
  {
    T_map *i=new T_map(this);
    QObject::connect(this,SIGNAL(addPointToSelectedObject()),i,SLOT(addPointToSelectedObject()));
    QObject::connect(this,SIGNAL(addPointToSelectedObject()),this,SIGNAL(addedRemoved()));
    return i;
  }
protected:
  void add()
  {
    createItem();
    //emit addedRemoved();
  }

  //QML objects list property
public:
  QQmlListProperty<T_map> objectsList()
  {
    return QQmlListProperty<T_map>(this,0,&objectsCount,&objectAt);
  }
private:
  static int objectsCount(QQmlListProperty<T_map>*list)
  {
    MissionItemCategory *category=static_cast<MissionItemCategory*>(list->object);
    if(category)return category->childCount();
    return 0;
  }
  static T_map* objectAt(QQmlListProperty<T_map> *list, int i)
  {
    MissionItemCategory *category=static_cast<MissionItemCategory*>(list->object);
    if(category)return qobject_cast<T_map*>(category->child(i));
    return 0;
  }
};
//=============================================================================
#endif
