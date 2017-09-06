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
#ifndef MapView_H
#define MapView_H
//-----------------------------------------------------------------------------
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QtCore>
#include "QMandala.h"
#include "MapTiles.h"
#include "ItemUav.h"
#include "ItemWpt.h"
#include "ItemText.h"
#include "ItemHome.h"
#include "ItemRw.h"
#include "ItemWind.h"
#include "ItemName.h"
//=============================================================================
class MapView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit MapView(QWidget *parent = 0);
  ~MapView();

  bool showBlockFrames;
  QPointF curLL,curXY,clkLL;
  QPoint curMS,clkMS;

  double scaleFactor;

  MapTiles mapTiles;
  ItemHome *iHome;
  ItemWind *iWind;
  ItemName *iName;

  ItemUav *currentUAV;
  QHash<QMandalaItem*,ItemUav*> iUAV;

  QPointF mapToSceneLL(double lat, double lon);
  QPointF mapLLFromScene(QPointF sceneXY);
  double mapMetersToScene(double m,double lat);
  double mapMetersFromScene(double x,double lat);
  QPointF mapNEtoScene(double N,double E,double lat,double lon);

  bool smooth;

  int fps,fps_cnt;

  QTimer bindTimer; //central timer for mandala binds update

  void setFollowUAV(bool v);
protected:
  void paintEvent(QPaintEvent *event);
  QElapsedTimer tmrFps;

  void mouseMoveEvent(QMouseEvent * e);
  void mousePressEvent(QMouseEvent * e);
  void mouseReleaseEvent(QMouseEvent * e);
  void wheelEvent(QWheelEvent * e);
  void mouseDoubleClickEvent(QMouseEvent * e);

  void scrollContentsBy(int dx,int dy);
private:
  QMandala *mandala;
  bool no_scene_sel_change;
  double zoomLevel;
  bool doZooming;
  QLineF pan_line;
  uint pan_frame;
  int wheelPos;
  bool bFollowUAV,bNoScrollEvent,bGoUAVpanning;
  void setCenter(const QPointF &pos,bool force_smooth=false);

private slots:
  void uavAdded(QMandalaItem *m);
  void mandalaCurrentChanged(QMandalaItem *m);

  void smoothZoomStep();
  void resetZoom(double zlevel);
  void pan_delta(bool start=false);
  void pan_pos(bool start=false);
  void pan_reset();

  void test();

public slots:
  void setShowBlockFrames(bool v);

  void goItem(QGraphicsItem *item);
  void goHome();
  void goUAV();
  void zoomIn();
  void zoomOut();
  void zoom(double sf);

  void followUAV(const QPointF &pos);


signals:
  void scaled();
  void panned();
  void updateStats();
  void mapClicked(MapTile *item);
};
//=============================================================================
#endif
