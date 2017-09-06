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
#ifndef ItemBase_H
#define ItemBase_H
#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsItem>
#include "QMandala.h"
class MapView;
//=============================================================================
class ItemBase : public QObject,public QGraphicsPixmapItem
{
  Q_OBJECT
public:
  ItemBase(MapView *view, QMandalaItem *mvar, const QPixmap &image=QPixmap(), bool movable=false, bool smoothMove=false);
  ~ItemBase();
  enum {
    Type=QGraphicsItem::UserType,
    TypeItemText,
    TypeItemUav,
    TypeItemHome,
    TypeItemStby,
    TypeItemWind,
    TypeItemName,
    TypeItemWpt,
    TypeItemRw,
    TypeItemTw,
    TypeItemPi,
    TypeItemArea,
    TypeItemAreaPoint,
    };
  virtual int type() const{return Type;}

  bool noUpdateBinds; //set true if must use dedicated mandala
  bool noChangeSig;
  //QObject *modelItem;

  virtual void setPosLL(QPointF ll);
  virtual void setPosXY(QPointF p);
  QPointF getPosLL(void);
  void addToScene(QGraphicsItem *item);
protected:
  QMandalaItem *mvar;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
  MapView *view;
  QPointF posLL;

  //var bindings
  void bind(const char * method, const QStringList &varNames);
  QMultiHash<QObject*,const char*> bindMap;
  void update_all_binds();
  //static QTimer bindTimer;

  void resetItemCache(QGraphicsItem *i); //qt flickr cache bug workaround
private:
  //smooth moving
  QTime posTime;
  QTimer posTimer;
  bool movable,smoothMove;
  QLineF pos_line;
  double posInterval;
  QList<QGraphicsItem*> sceneItems;

  QList<QMetaObject::Connection>mcon;
private slots:
  void mandalaCurrentChanged(QMandalaItem *m);
  void bindTimeout();
  void fieldChanged();
  void posTick();
signals:
  void moved();
  void selected(bool);
  void smoothMoved(const QPointF &pos);
  void update_all_binds_signal();
  void apcfgChanged();
public slots:
  virtual void mandalaUpdated(uint var_idx);
};
//=============================================================================
#endif
