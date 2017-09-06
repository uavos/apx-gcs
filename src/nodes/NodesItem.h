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
#ifndef NodesItem_H
#define NodesItem_H
//=============================================================================
#include <QtCore>
#include <QtWidgets>
#include <QDomDocument>
#include <QModelIndex>
//=============================================================================
class NodesItem: public QObject
{
  Q_OBJECT
public:
  NodesItem(NodesItem *parent=NULL,QString name=QString(), QString descr=QString());
  virtual ~NodesItem(){}
  //virtual methods
  virtual QVariant data(int column,int role = Qt::DisplayRole) const;
  virtual QVariant getValue(void) const;
  virtual bool setValue(const QVariant &value);

  virtual int getConfSize(void) const;

  virtual bool isModified(void) const;
  virtual bool isValid(void) const;
  virtual bool isZero(void) const;
  virtual bool isReconf(void) const;

  virtual bool isUpgrading(void) const;
  virtual bool isUpgradable(void) const;

  virtual uint size(void) const; //items cnt
  virtual uint sizePending(void) const; //invalid or modified items cnt

  //status
  virtual void restore(void);
  virtual void invalidate(void);

  //methods
  void appendChild(NodesItem *child);
  void removeChild(NodesItem *child);
  NodesItem * find(QString iname, NodesItem *scope);
  void emit_changed(){emit changed();}


  //internal tree
  QList<NodesItem*> childItems;
  NodesItem *parentItem;
  NodesItem *child(int row);
  int childCount() const;
  int columnCount() const;
  bool setData(int column, const QVariant & value);
  NodesItem *parent();
  virtual int row() const;
  virtual Qt::ItemFlags flags(int column) const;

  //columns
  enum{tc_field,tc_value,tc_descr};

  //item type
  typedef enum{ it_root,it_vehicle,it_ngroup,it_node,it_group,it_field,it_fpart }_item_type;
  _item_type item_type;

  QModelIndex model_index;

  //item parameters
  QString           name;
  QString           descr;
signals:
  void changed(void);

public slots:
  virtual void sync(void);
  virtual void stats(void);
  virtual void upload(void);

  virtual void saveToXml(QDomNode dom) const;
  virtual void loadFromXml(QDomNode dom);

  virtual void clear(void);
  virtual void stop(void);

};
//=============================================================================
#endif
