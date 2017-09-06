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
#include <QStyleOptionGraphicsItem>
#include "ItemBase.h"
#include "QMandala.h"
#include "MapView.h"
//=============================================================================
ItemBase::ItemBase(MapView *view, QMandalaItem *mvar, const QPixmap &image, bool movable, bool smoothMove)
: QObject(),
  QGraphicsPixmapItem(image),
  noUpdateBinds(false),noChangeSig(false),
  mvar(mvar),
  view(view),
  posLL(QPointF(1000,0)),movable(movable),smoothMove(smoothMove)
{
  setShapeMode(BoundingRectShape);
  if(!image.isNull()){
    setFlag(ItemIgnoresTransformations);
    setOffset(-image.width()/2.0,-image.height()/2.0);
    setCacheMode(DeviceCoordinateCache);
    //setCacheMode(ItemCoordinateCache);
    //if(view->smooth) setTransformationMode(Qt::SmoothTransformation);
  }
  if(movable){
    setCursor(Qt::PointingHandCursor);
    setFlag(ItemIsSelectable);
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
  }
  //setScale(1.0/view->scaleFactor);

  noChangeSig=false;
  view->scene()->addItem(this);
  posTimer.setInterval(5);
  posTimer.setSingleShot(true);
  connect(&posTimer,SIGNAL(timeout()),this,SLOT(posTick()));

  connect(QMandala::instance(),SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(mandalaCurrentChanged(QMandalaItem*)));
  mandalaCurrentChanged(QMandala::instance()->current);
}
ItemBase::~ItemBase()
{
  foreach(QGraphicsItem *i,sceneItems){
    if(view->scene()==i->scene())
      view->scene()->removeItem(i);
    //else qDebug()<<i;
    delete i;
  }
  //qDeleteAll(bindTimers);
}
//=============================================================================
void ItemBase::mandalaCurrentChanged(QMandalaItem *m)
{
  foreach(QMetaObject::Connection c,mcon) disconnect(c);
  mcon.clear();
  mcon.append(connect(m,SIGNAL(updated(uint)),this,SLOT(mandalaUpdated(uint))));
  mcon.append(connect(m,SIGNAL(apcfgChanged()),this,SIGNAL(apcfgChanged())));
  if(noUpdateBinds)return;
  //update binds
  if(!bindMap.size())return;
  QMultiHash<QObject*,const char*> newBindMap;
  foreach(QObject *f,bindMap.uniqueKeys()){
    disconnect(f,SIGNAL(changed()),this,SLOT(fieldChanged()));
    QMandalaField *f_new=m->field(static_cast<QMandalaField*>(f)->name());
    connect(f_new,SIGNAL(changed()),this,SLOT(fieldChanged()));
    foreach(const char* method,bindMap.values(f)){
      newBindMap.insertMulti(f_new,method);
    }
  }
  bindMap=newBindMap;
  update_all_binds();
}
//=============================================================================
void ItemBase::addToScene(QGraphicsItem *item)
{
  if(item->scene()!=view->scene())
    view->scene()->addItem(item);
  sceneItems.append(item);
}
//=============================================================================
void ItemBase::setPosLL(QPointF ll)
{
  if(posLL==ll)return;
  posLL=ll;
  setPosXY(view->mapToSceneLL(ll.x(),ll.y()));
}
void ItemBase::setPosXY(QPointF p)
{
  noChangeSig=true;
  pos_line.setP1(pos());
  //QPointF lp=mapFromScene(p)-pos();
  //uint len=sqrt(pow(lp.x(),2)+pow(lp.y(),2));
  if(smoothMove){// && len < 5000){
    pos_line.setP2(p);
    posInterval=posInterval*0.9+0.1*posTime.elapsed();
    if(posInterval<10)posInterval=10;
    else if(posInterval>300)posInterval=300;
    //qDebug()<<posInterval;
    posTime.start();
    if(!posTimer.isActive())posTimer.start();
  }else{
    setPos(p);
    emit smoothMoved(p);
  }
  noChangeSig=false;
}
QPointF ItemBase::getPosLL(void)
{
  posLL=view->mapLLFromScene(pos());
  return posLL;
}
//=============================================================================
QVariant ItemBase::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if((!noChangeSig) && change == ItemPositionChange) {
    QPointF p(QGraphicsItem::itemChange(change,value).toPointF());
    posLL=view->mapLLFromScene(p);
    emit moved();
    return p;
  }else if(change == ItemSelectedHasChanged) {
    emit selected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}
//=============================================================================
void ItemBase::mandalaUpdated(uint var_idx)
{
  Q_UNUSED(var_idx)
}
//=============================================================================
void ItemBase::posTick()
{
  double t=0.5*(double)posTime.elapsed()/(double)posInterval;
  if(t>1)t=1;
  QPointF p=pos_line.pointAt(t);
  noChangeSig=true;
  setPos(p);
  emit smoothMoved(p);
  noChangeSig=false;
  if(t<1) posTimer.start();
}
//=============================================================================
//QTimer ItemBase::bindTimer;
//=============================================================================
void ItemBase::bind(const char * method, const QStringList &varNames)
{
  if(!bindMap.size()){
    connect(&view->bindTimer,SIGNAL(timeout()),this,SLOT(bindTimeout()),Qt::QueuedConnection);
    view->bindTimer.setSingleShot(true);
  }
  foreach(QString v,varNames){
    QMandalaField *f=mvar->field(v);
    if(!bindMap.contains(f))connect(f,SIGNAL(changed()),this,SLOT(fieldChanged()));
    bindMap.insertMulti(f,method);
    connect(this,SIGNAL(update_all_binds_signal()),this,method);
  }
}
void ItemBase::bindTimeout()
{
  view->bindTimer.disconnect();
  connect(&view->bindTimer,SIGNAL(timeout()),this,SLOT(bindTimeout()),Qt::QueuedConnection);
}
void ItemBase::fieldChanged()
{
  if(!view->bindTimer.isActive()){
    view->bindTimer.start(20);
  }
  QObject *f=sender();
  if(!bindMap.contains(f))return;
  foreach(const char *method,bindMap.values(f)){
    connect(&view->bindTimer,SIGNAL(timeout()),this,method);
  }
}
void ItemBase::update_all_binds()
{
  emit update_all_binds_signal();
}
//=============================================================================
void ItemBase::resetItemCache(QGraphicsItem *i)
{
  //reset cache bugfix (flick)
  CacheMode cm=i->cacheMode();
  i->setCacheMode(QGraphicsItem::NoCache);
  i->setCacheMode(cm);
}
//=============================================================================

