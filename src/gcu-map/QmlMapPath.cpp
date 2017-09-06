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
#include "QmlMapPath.h"
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgflatcolormaterial.h>
//=============================================================================
QmlMapPath::QmlMapPath(QQuickItem *parent)
  : QQuickItem(parent),
  m_path(NULL),m_provider(NULL),
  m_lineWidth(1),m_color(Qt::white)
{
  setFlag(ItemHasContents, true);
}
//=============================================================================
MissionPath * QmlMapPath::path()
{
  return m_path;
}
void QmlMapPath::setPath(MissionPath *v)
{
  if(m_path==v)return;
  m_path=v;
  connect(m_path,SIGNAL(pathChanged()),this,SLOT(updatePoints()));
  updatePoints();
  emit pathChanged();
}
QmlMap * QmlMapPath::provider()
{
  return m_provider;
}
void QmlMapPath::setProvider(QmlMap *v)
{
  if(m_provider==v)return;
  m_provider=v;
  connect(m_provider,&QmlMap::levelChanged,this,&QmlMapPath::updatePoints);
  connect(m_provider,&QmlMap::mapScaleFactorChanged,this,&QmlMapPath::update);
  updatePoints();
  emit pathChanged();
}
//=============================================================================
int QmlMapPath::lineWidth()
{
  return m_lineWidth;
}
void QmlMapPath::setLineWidth(int v)
{
  if(m_lineWidth==v)return;
  m_lineWidth=v;
  emit lineWidthChanged();
  update();
}
QColor QmlMapPath::color()
{
  return m_color;
}
void QmlMapPath::setColor(QColor v)
{
  if(m_color==v)return;
  m_color=v;
  emit colorChanged();
  update();
}
QPoint QmlMapPath::shift()
{
  return m_shift;
}
void QmlMapPath::setShift(QPoint v)
{
  if(m_shift==v)return;
  m_shift=v;
  emit shiftChanged();
  updatePoints();
}
//=============================================================================
void QmlMapPath::updatePoints()
{
  if(m_provider==NULL || m_path==NULL)return;
  bool bUpd=false;
  QPointF p0;
  int pcnt=0;
  int cnt=m_path->path().size()-1;
  for(int i=0;i<=cnt;i++){ // const QPointF &ll,m_path->path()){
    const QPointF &ll=m_path->path().at(i);
    QPointF p;
    const double constSceneXY=m_provider->levelTiles()*m_provider->tileSize();
    p.setX(m_provider->lonToX(ll.y())*constSceneXY-m_shift.x());
    p.setY(m_provider->latToY(ll.x())*constSceneXY-m_shift.y());
    if((i<cnt) && (!p0.isNull()) && QPointF(p-p0).manhattanLength()<5)continue;
    p0=p;
    //update points
    if(points.size()<=pcnt){
      points.append(p);
      bUpd=true;
    }else if(points.at(pcnt)!=p){
      points[pcnt]=p;
      bUpd=true;
    }
    pcnt++;
  }
  while(points.size()>pcnt){
    points.removeLast();
    bUpd=true;
  }
  if(bUpd)update();
}
//=============================================================================
QSGNode *QmlMapPath::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
  QSGGeometryNode *node = 0;
  QSGGeometry *geometry = 0;
  if(!oldNode) {
    node = new QSGGeometryNode;
    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), points.size());
    geometry->setLineWidth(m_lineWidth*m_provider->mapScaleFactor());
    geometry->setDrawingMode(GL_LINE_STRIP);
    geometry->setVertexDataPattern(QSGGeometry::AlwaysUploadPattern);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    material = new QSGFlatColorMaterial;
    material->setColor(m_color);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
  }else{
    node = static_cast<QSGGeometryNode *>(oldNode);
    geometry = node->geometry();
    geometry->setLineWidth(m_lineWidth*m_provider->mapScaleFactor());
    geometry->allocate(points.size());
    material->setColor(m_color);
  }
  QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
  for(int i=0;i<points.size();i++){
    const QPointF &p=points.at(i);
    vertices[i].set(p.x(),p.y());
  }
  node->markDirty(QSGNode::DirtyGeometry);
  //qDebug()<<bounds;//points;//node;
  return node;
}
//=============================================================================
//=============================================================================
