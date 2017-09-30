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
#include <math.h>
#include <QtCore>
#include <QApplication>
#include <QWheelEvent>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <QProgressDialog>
#include <QDesktopWidget>
#include <QAction>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QGLWidget>
#include <QPixmapCache>

#include "MapView.h"
#include "QMandala.h"
//-----------------------------------------------------------------------------
//=============================================================================
MapView::MapView(QWidget *parent)
  : QGraphicsView(parent),
    scaleFactor(1.0),
    mapTiles(this),
    currentUAV(NULL)
{
  mandala=qApp->property("Mandala").value<QMandala*>();

  if(!QSettings().contains("smooth_uav")) QSettings().setValue("smooth_uav",false);
  if(!QSettings().contains("smooth_map")) QSettings().setValue("smooth_map",true);
  smooth=QSettings().value("smooth_map").toBool();

  no_scene_sel_change=false;
  doZooming=false;

  fps=fps_cnt=0;

  showBlockFrames=false;
  pan_frame=0;
  wheelPos=0;
  bFollowUAV=bNoScrollEvent=bGoUAVpanning=false;

  horizontalScrollBar()->setSingleStep(1);
  verticalScrollBar()->setSingleStep(1);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

#ifdef __APPLE__
  //QGLFormat fmt;
  //fmt.setSampleBuffers(true);
  //fmt.setSamples(2);
  //if(QSettings().value("opengl",true).toBool()&&QGLFormat::openGLVersionFlags())
    //setViewport(new QGLWidget());//fmt));
#endif

  setAutoFillBackground(false);
  //setBackgroundBrush(Qt::black);
  QPixmapCache::setCacheLimit(256*1024); //MB

  setRenderHint(QPainter::Antialiasing,smooth);
  setRenderHint(QPainter::SmoothPixmapTransform,smooth);
  setRenderHint(QPainter::TextAntialiasing,false);

  setAttribute(Qt::WA_TranslucentBackground, false);

  setOptimizationFlags(QGraphicsView::DontClipPainter);
  setOptimizationFlags(QGraphicsView::DontSavePainterState);
  setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing);
  setCacheMode(QGraphicsView::CacheBackground);
  setViewportUpdateMode(MinimalViewportUpdate);
  //setViewportUpdateMode(SmartViewportUpdate);

  setDragMode(QGraphicsView::ScrollHandDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setResizeAnchor(AnchorViewCenter);

  QGraphicsScene *scene=new QGraphicsScene(0,0,MapTiles::MaxTiles,MapTiles::MaxTiles);
  //scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
  //scene->setBspTreeDepth(16);
  scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  scene->setBackgroundBrush(Qt::NoBrush);
  mapTiles.setScene(scene);
  setScene(scene);


  connect(this,SIGNAL(panned()),&mapTiles,SLOT(updateTiles()),Qt::QueuedConnection);
  connect(&mapTiles,SIGNAL(downloaded(QString)),SIGNAL(updateStats()));

  //add additional icons..
  uavAdded(mandala->local); //local
  iHome=new ItemHome(this);
  iWind=new ItemWind(this);
  iName=new ItemName(this);
  foreach(QMandalaItem *m,mandala->items)
    uavAdded(m);


  connect(mandala,SIGNAL(uavAdded(QMandalaItem*)),this,SLOT(uavAdded(QMandalaItem*)));
  connect(mandala,SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(mandalaCurrentChanged(QMandalaItem*)));

  resetZoom(32);
  goHome();
  mandala->current->emitUpdated();
}
void MapView::test()
{
  translate(0.1,0.01);
  QTimer::singleShot(1,this,SLOT(test()));;
}
//=============================================================================
MapView::~MapView()
{
  scene()->deleteLater();
}
//=============================================================================
void MapView::uavAdded(QMandalaItem *m)
{
  ItemUav *i;
  if(iUAV.contains(m))i=iUAV.value(m);
  else{
    i=new ItemUav(this,m);
    iUAV.insert(m,i);
  }
  mandalaCurrentChanged(mandala->current);
}
void MapView::mandalaCurrentChanged(QMandalaItem *m)
{
  if(iUAV.contains(m))currentUAV=iUAV.value(m);
  QTimer::singleShot(5000,this,SLOT(goUAV()));
  foreach(QMandalaItem *mi,iUAV.keys()){
    iUAV.value(mi)->setCurrent(m==mi);
  }
}
//=============================================================================
void MapView::paintEvent(QPaintEvent *event)
{
  if(!tmrFps.isValid())tmrFps.start();
  QGraphicsView::paintEvent(event);
  fps_cnt++;
  qint64 t=tmrFps.elapsed();
  if(t<1000)return;
  tmrFps.start();
  fps=fps_cnt/(t/1000.0);
  fps_cnt=0;
  if(fps>99)fps=99;
}
//=============================================================================
QPointF MapView::mapToSceneLL(double lat, double lon)
{
  double X = MapTiles::MaxTiles2+lon*MapTiles::MaxTiles/360.0;
  double z=sin(M_PI/180.0*lat);
  double Y = MapTiles::MaxTiles2-0.5*log((1.0+z)/(1.0-z))*MapTiles::MaxTiles/(2.0*M_PI);
  return QPointF(X,Y);
}
//----------------------------
QPointF MapView::mapLLFromScene(QPointF sceneXY)
{
  double lon = (sceneXY.x()-MapTiles::MaxTiles2)/(MapTiles::MaxTiles/360.0);
  double z = (sceneXY.y() - MapTiles::MaxTiles2)/-(MapTiles::MaxTiles/(2.0*M_PI));
  double lat = (2.0 * atan(exp(z)) - M_PI/2.0) * 180.0/M_PI;
  return QPointF(lat,lon);
}
//----------------------------
double MapView::mapMetersToScene(double m,double lat)
{
  const double r=EARTH_RADIUS*cos(lat*D2R);
  return m*MapTiles::MaxTiles/(r*2.0*M_PI);
}
//----------------------------
double MapView::mapMetersFromScene(double x,double lat)
{
  return x/mapMetersToScene(1,lat);
}
//----------------------------
QPointF MapView::mapNEtoScene(double N,double E,double lat,double lon)
{
  const Point &ll=mandala->current->ne2ll(Point(N,E),Vect(lat,lon,0));
  return mapToSceneLL(ll[0],ll[1]);
}
//=============================================================================
void MapView::goItem(QGraphicsItem *item)
{
  if(item)setCenter(item->pos());
}
//=============================================================================
void MapView::goHome()
{
  setCenter(mapToSceneLL(mandala->current->home_pos[0],mandala->current->home_pos[1]));
  setFollowUAV(false);
  bGoUAVpanning=bNoScrollEvent=false;
}
//=============================================================================
void MapView::goUAV()
{
  if(mandala->current->gps_pos.isNull())return;
  setCenter(currentUAV->pos());//mapToSceneLL(mandala->current->gps_pos[0],mandala->current->gps_pos[1]));
  bGoUAVpanning=true;
}
void MapView::followUAV(const QPointF &pos)
{
  if(!bFollowUAV)return;
  if(sender()!=currentUAV)return;
  bNoScrollEvent=true;
  centerOn(pos);
  bNoScrollEvent=false;
}
void MapView::setFollowUAV(bool v)
{
  if(bFollowUAV==v)return;
  bFollowUAV=v;
  if(bFollowUAV){
    setRenderHint(QPainter::Antialiasing,false);
    setRenderHint(QPainter::SmoothPixmapTransform,false);
  }else{
    setRenderHint(QPainter::Antialiasing,smooth);
    setRenderHint(QPainter::SmoothPixmapTransform,smooth);
  }
}
//=============================================================================
void MapView::zoomIn()
{
  zoom(2.0);
}
void MapView::zoomOut()
{
  zoom(0.5);
}
void MapView::zoom(double sf)
{
  zoomLevel=zoomLevel*sf;
  //limit, align by factor of two
  if(zoomLevel<=0.008){
      zoomLevel=0.008;
    }else if(zoomLevel>=(256*4)){
      zoomLevel=(256*4);
    }else if(zoomLevel>1.0){
      sf=2.0;
      while(1){
          if(zoomLevel<=sf){
              zoomLevel=sf;
              break;
            }
          sf=sf*2.0;
        }
    }else if(zoomLevel<1.0){
      sf=0.5;
      while(1){
          if(zoomLevel>=sf){
              zoomLevel=sf;
              break;
            }
          sf=sf*0.5;
        }
    }
  smoothZoomStep();
}
//=============================================================================
void MapView::smoothZoomStep()
{
  doZooming=true;
  const double step=0.1;
  const double stepIn=1.0+step;
  const double stepOut=1.0-step;

  QTransform tm=transform();
  double sf=tm.m11();
  if(zoomLevel>sf){
      sf=sf*stepIn;
      if(sf<zoomLevel){
          scale(stepIn,stepIn);
          QTimer::singleShot(20,this,SLOT(smoothZoomStep()));
          return;
        }
    }else{
      sf=sf*stepOut;
      if(sf>zoomLevel){
          scale(stepOut,stepOut);
          QTimer::singleShot(20,this,SLOT(smoothZoomStep()));
          return;
        }
    }
  //scale done
  resetZoom(zoomLevel);
}
//=============================================================================
void MapView::resetZoom(double zlevel)
{
  zoomLevel=zlevel;
  QTransform tm=transform();
  tm.setMatrix(zoomLevel,tm.m12(),tm.m13(),tm.m21(),zoomLevel,tm.m23(),tm.m31(),tm.m32(),tm.m33());
  setTransform(tm);
  doZooming=false;
  if(mapTiles.updateLevel(transform().m11()*scaleFactor))
    mapTiles.updateTiles();
  emit scaled();
}
//=============================================================================
void MapView::scrollContentsBy(int dx,int dy)
{
  QGraphicsView::scrollContentsBy(dx,dy);
  emit panned();
  //panning
  if(!pan_frame){
      QPoint p=pan_line.p2().toPoint();
      if(abs(dx)>=5)p.setX(dx);
      else if(dx!=0)p.setX(0);
      if(abs(dy)>=5)p.setY(dy);
      else if(dy!=0)p.setY(0);
      pan_line.setP2(p);
    }
  //if(!doZooming) mapTiles.updateTiles();
  if(bNoScrollEvent)return;
  setFollowUAV(false);
}
void MapView::pan_delta(bool start)
{
  if(start)pan_frame=100;
  else if(!pan_frame)return;
  pan_frame=pan_frame*0.9;
  QPoint p=pan_line.pointAt(pan_frame/100.0).toPoint();
  if(p.isNull()){
      pan_frame=0;
      return;
    }
  centerOn(mapToScene(QPoint(width()/2,height()/2)-p));
  QTimer::singleShot(10,this,SLOT(pan_delta()));
}
void MapView::pan_pos(bool start)
{
  if(start)pan_frame=100;
  else if(!pan_frame){
    if(bGoUAVpanning){
      bGoUAVpanning=false;
      setFollowUAV(true);
    }
    return;
  }
  pan_frame=pan_frame*0.9;
  QPointF p=pan_line.pointAt(pan_frame/100.0);
  centerOn(p);
  QTimer::singleShot(10,this,SLOT(pan_pos()));
}
void MapView::pan_reset()
{
  pan_frame=0;
  pan_line=QLineF();
}
void MapView::setCenter(const QPointF &pos,bool force_smooth)
{
  if(pan_line.p1()==pos)return;
  QPoint sp=QPoint(width()/2,height()/2);
  if(!force_smooth){
      QPoint lp=mapFromScene(pos)-sp;
      uint len=sqrt(pow(lp.x(),2)+pow(lp.y(),2));
      if(len<10 || len > 5000){
          pan_reset();
          centerOn(pos);
          pan_pos();
          return;
        }
    }
  pan_line.setP2(mapToScene(sp));
  pan_line.setP1(pos);
  pan_pos(true);
}
//=============================================================================
void MapView::mousePressEvent(QMouseEvent * e)
{
  clkMS=e->pos();
  clkLL=mapLLFromScene(mapToScene(clkMS));
  pan_reset();
  QGraphicsView::mousePressEvent(e);
}
void MapView::mouseReleaseEvent(QMouseEvent * e)
{
  QGraphicsView::mouseReleaseEvent(e);
  if (dragMode()==QGraphicsView::ScrollHandDrag){
      //if(QSettings().value("MapSmoothScroll").toBool())
        //pan_delta(true);
      return;
    }
  QList<QGraphicsItem *> ilist=items(e->pos());
  if(!ilist.size())return;
  foreach(QGraphicsItem *item,ilist){
      //qDebug("click %i",item->type());
      if (qgraphicsitem_cast<MapTile *>(item)){
          emit mapClicked((MapTile*)item);
          //qDebug("clicked");
          break;
        }
    }

  /*QGraphicsItem *item = itemAt(e->pos());
  if (!item)return;
  if (qgraphicsitem_cast<MapTile *>(item))
    emit mapClicked((MapTile*)item);*/
}
void MapView::mouseMoveEvent(QMouseEvent * e)
{
  QGraphicsView::mouseMoveEvent(e);
  QCoreApplication::processEvents(); //fix qml freeze on macos
  if(doZooming)return;
  curXY=mapToScene(e->pos());
  curLL=mapLLFromScene(curXY);
  emit updateStats();
}
void MapView::wheelEvent(QWheelEvent * e)
{
  e->accept();
  pan_reset();
  wheelPos+=e->delta();
  if(abs(wheelPos)<400)return;
  wheelPos=0;
  if(doZooming)return;
  if (e->delta()>0)zoomIn();
  else zoomOut();
}
void MapView::mouseDoubleClickEvent(QMouseEvent * e)
{
  QGraphicsView::mouseDoubleClickEvent(e);
  zoomIn();
}
//=============================================================================
void MapView::setShowBlockFrames(bool v)
{
  if(showBlockFrames==v)return;
  showBlockFrames=v;
  mapTiles.updateTiles();
}
//=============================================================================



