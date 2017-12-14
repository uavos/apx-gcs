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
#ifndef ItemUav_H
#define ItemUav_H
#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsDropShadowEffect>
#include "QMandalaItem.h"
#include "ItemBase.h"
#include "ItemText.h"
#include "Mandala.h"
//=============================================================================
class ItemUav : public ItemBase
{
  Q_OBJECT
public:
  ItemUav(MapView *view,QMandalaItem *mvar);
  enum {Type=TypeItemUav};
  int type() const{return Type;}

  double dist_file;
  double dist;
private:
  bool m_current;
  bool m_showTrace;
  bool m_showHdg;
  QPixmap uavPixmap; //to cache projection
  QGraphicsPixmapItem uavIcon;
  QGraphicsPixmapItem *ahrsIcon;
  QGraphicsPixmapItem hdgWheel;
  QGraphicsPixmapItem hdgArrow;
  QGraphicsPathItem pathCam,pathCam2;
  double camSpan,camSpan2;
  bool camRelative;
  int theta[3];

  static QPixmap getIconPixmap(QMandalaItem *m);

  QGraphicsEllipseItem *camTarget;


  ItemText txtInfo;
  QTimer statsTimer;

  QGraphicsPathItem pathCmdCrs,pathHdg,pathCrs;

  QGraphicsEllipseItem *cmd_NE;
  QGraphicsEllipseItem *veCircle;
  QGraphicsEllipseItem *veCircleGS;
  QGraphicsEllipseItem *stbyCircle;
  QGraphicsEllipseItem *restrictedCircle;
  QGraphicsLineItem *testLine;
  Vect trace_llh_prev;

  //estimated reachable ground
  double gPerf;

  QGraphicsItemGroup *traceFromFile;
  QGraphicsItemGroup *traceCurrent;
  QList<QPointF> traceData;
  QList<QPointF> traceDataFromFile;
  QPointF trace_s;

  //mission sequence
  bool missionNextUpd;

signals:
  void statsUpdated();
private slots:
  //var bindings
  void update_theta();
  void update_gPerf();
  void update_pos();
  void update_cmdNE();
  void update_LD();
  void update_Restricted();
  void update_stats();
  void update_stats_do();
  void update_stby();


  //other
  void mandalaFileLoaded();
  void trace(double lat, double lon, double hmsl);
  void updateTraceFromFile(void);
  void updateTraceCurrent(void);
  void updateTrace(const QList<QPointF> &data,QGraphicsItem *item,const QPen &pen);

  void updateLinkState(void);
public slots:
  void clearTrace();
  void clearTraceF();
  void setShowTrace(bool show=true);
  void setShowHdg(bool v);

  void setCurrent(bool v);
};
//=============================================================================
#endif
