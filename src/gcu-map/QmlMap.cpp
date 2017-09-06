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
#include "QmlMap.h"
#include "QMandala.h"
#include "QmlMapTiles.h"
#include "QmlMapPath.h"
#include "QmlMissionModel.h"
#include "MissionPath.h"
//=============================================================================
QmlMap::QmlMap(QQuickItem *parent)
  : QQuickItem(parent)
{
  m_maxLevel=21;
  m_maxTiles=pow(2,m_maxLevel);
  m_tileSize=256;
  m_level=-1;
  m_mapScaleFactor=0;
  m_itemScaleFactor=0;
  setMapScaleFactor(1);
#ifdef Q_OS_ANDROID
  setItemScaleFactor(2.5);
#else
  setItemScaleFactor(1);
#endif
  setLevel(16);
  connect(this,&QmlMap::windowChanged,this,&QmlMap::onWindowChanged);
}
void QmlMap::componentComplete()
{
  //QQmlEngine *e=qmlEngine(this);
  //qDebug()<<e;
  //e->addImageProvider("maps",&imageProvider);
  //connect(&imageProvider,SIGNAL(downloaded(QString)),this,SLOT(tileDownloaded(QString)));
  QQuickItem::componentComplete();
}
void QmlMap::registerTypes()
{
  //can be reated from Qml
  qmlRegisterType<QmlMap>("com.uavos.map", 1, 0, "QmlMap");
  qmlRegisterType<QmlMapTiles>("com.uavos.map", 1, 0, "QmlMapTiles");
  qmlRegisterType<QmlMapPath>("com.uavos.map", 1, 0, "QmlMapPath");
  qmlRegisterType<QmlMissionModel>("com.uavos.map", 1, 0, "QmlMissionModel");

  //can be accessed in qml
  qmlRegisterType<MissionModel>();
  qmlRegisterType<MissionPath>();
  qmlRegisterType<MissionItem>();
  qmlRegisterType<MissionItemWp>();
  qmlRegisterType<MissionItemRw>();
  qmlRegisterType<MissionItemTw>();
  qmlRegisterType<MissionItemPi>();
  qmlRegisterType<MissionItemArea>();
}
void QmlMap::onWindowChanged(QQuickWindow *window)
{
  //qDebug()<<window;
  //scaleFactor adjust for screen & platform
  connect(window,&QQuickWindow::screenChanged,this,&QmlMap::onScreenChanged);
  onScreenChanged(window->screen());
}
void QmlMap::onScreenChanged(QScreen *screen)
{
  qDebug()<<screen<<screen->physicalDotsPerInch();
  qreal dpi=screen->physicalDotsPerInch();
  if(dpi<=200)setMapScaleFactor(1);
  else setMapScaleFactor(dpi/200);
}
//=============================================================================
// PROPERTIES
//=============================================================================
QRectF QmlMap::visibleArea() const
{
  return m_visibleArea;
}
void QmlMap::setVisibleArea(const QRectF &v)
{
  if(m_visibleArea==v)return;
  m_visibleArea=v;
  //qDebug()<<v;
  emit visibleAreaChanged();
  updateCenter();
  updateMouse();
}
void QmlMap::updateCenter()
{
  double cv=m_tileSize*m_levelTiles;
  double cX=(m_visibleArea.x()+m_shift.x()*m_tileSize+m_visibleArea.width()/2.0)/cv;
  double cY=(m_visibleArea.y()+m_shift.y()*m_tileSize+m_visibleArea.height()/2.0)/cv;
  setCenter(QPointF(cX,cY));
}
QPointF QmlMap::center() const
{
  return m_center;
}
void QmlMap::setCenter(QPointF v)
{
  if(m_center==v)return;
  m_center=v;
  //qDebug()<<v;
  emit centerChanged();
}
//=============================================================================
QPoint QmlMap::mousePos() const
{
  return m_mousePos;
}
void QmlMap::setMousePos(QPoint v)
{
  v-=m_visibleArea.topLeft().toPoint(); //parent is Flickable
  if(v.x()<0 || v.x()>=m_visibleArea.width())return;
  if(v.y()<0 || v.y()>=m_visibleArea.height())return;

  /*if(v.x()<0)v.setX(0);
  else if(v.x()>=m_visibleArea.width())v.setX(m_visibleArea.width()-1);
  if(v.y()<0)v.setY(0);
  else if(v.y()>=m_visibleArea.height())v.setY(m_visibleArea.height()-1);*/
  if(m_mousePos==v)return;
  m_mousePos=v;
  //qDebug()<<v;
  emit mousePosChanged();
  updateMouse();
}
void QmlMap::updateMouse()
{
  QPointF mpos(m_mousePos);
  mpos+=m_visibleArea.topLeft();
  mpos+=m_shift*m_tileSize;
  mpos/=(double)(m_tileSize*m_levelTiles);
  if(m_mouse==mpos)return;
  m_mouse=mpos;
  emit mouseChanged();
  QPointF v(yToLat(m_mouse.y()),xToLon(m_mouse.x()));
  if(m_mouseLL==v)return;
  m_mouseLL=v;
  emit mouseLLChanged();
}
QPointF QmlMap::mouse() const
{
  return m_mouse;
}
QPointF QmlMap::mouseLL() const
{
  return m_mouseLL;
}
//=============================================================================
int QmlMap::level() const
{
  return m_level;
}
void QmlMap::setLevel(int v)
{
  if(v<1)v=1;
  else if(v>m_maxLevel)v=m_maxLevel;
  if(m_level==v)return;
  m_level=v;
  m_levelTiles=pow(2,m_level);
  emit levelChanged();
  emit levelTilesChanged();
}
int QmlMap::levelTiles() const
{
  return m_levelTiles;
}
int QmlMap::maxLevel() const
{
  return m_maxLevel;
}
int QmlMap::maxTiles() const
{
  return m_maxTiles;
}
int QmlMap::tileSize() const
{
  return m_tileSize;
}
//=============================================================================
QPoint QmlMap::shift() const
{
  return m_shift;
}
void QmlMap::setShift(QPoint v)
{
  if(m_shift==v)return;
  m_shift=v;
  //qDebug()<<"Shift: "<<v;
  emit shiftChanged();
  updateCenter();
}
//=============================================================================
qreal QmlMap::mapScaleFactor() const
{
  return m_mapScaleFactor;
}
void QmlMap::setMapScaleFactor(qreal v)
{
  if(m_mapScaleFactor==v || v<0.1 || v>10)return;
  m_mapScaleFactor=v;
  int sz=floor(256.0*v);
  if(m_tileSize!=sz){
    m_tileSize=sz;
    //qDebug()<<"tileSizeChanged: "<<sz;
    emit tileSizeChanged(sz);
  }
  emit mapScaleFactorChanged();
}
qreal QmlMap::itemScaleFactor() const
{
  return m_itemScaleFactor;
}
void QmlMap::setItemScaleFactor(qreal v)
{
  if(m_itemScaleFactor==v || v<0.1 || v>10)return;
  m_itemScaleFactor=v;
  emit itemScaleFactorChanged();
}
//=============================================================================
//=============================================================================
double QmlMap::latToY(double lat)
{
  double z=sin(M_PI/180.0*lat);
  return ((1.0/2.0)-0.5*log((1.0+z)/(1.0-z))*1.0/(2.0*M_PI));
}
double QmlMap::lonToX(double lon)
{
  return ((1.0/2.0)+lon*(1.0/360.0));
}
//=============================================================================
double QmlMap::yToLat(double y)
{
  double z = (y - 1.0/2.0)/-(1.0/(2.0*M_PI));
  return (2.0 * atan(exp(z)) - M_PI/2.0) * 180.0/M_PI;
}
double QmlMap::xToLon(double x)
{
  return (x-1.0/2.0)/(1.0/360.0);
}
double QmlMap::xToMeters(double y)
{
  return 1.0/metersToX(y);
}
double QmlMap::metersToX(double y)
{
  const double r=EARTH_RADIUS*cos(yToLat(y)*D2R);
  return 1.0/(r*2.0*M_PI);
}
//=============================================================================
