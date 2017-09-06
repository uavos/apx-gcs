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
#ifndef QMLMAP_H
#define QMLMAP_H
#include <QtCore>
#include <QtNetwork>
#include <QtQuick>
//=============================================================================
class QmlMap : public QQuickItem
{
  Q_OBJECT
public:
  QmlMap(QQuickItem *parent=0);

  static void registerTypes();

protected:
  void componentComplete();

//exposed methods
public:
  Q_INVOKABLE double latToY(double lat);
  Q_INVOKABLE double lonToX(double lon);

  Q_INVOKABLE double yToLat(double y);
  Q_INVOKABLE double xToLon(double x);
  Q_INVOKABLE double xToMeters(double y);
  Q_INVOKABLE double metersToX(double y);

private:
  void updateCenter();
  void updateMouse();

private slots:
  void onWindowChanged(QQuickWindow *window);
  void onScreenChanged(QScreen *screen);
//PROPERTIES
public:
  Q_PROPERTY(QRectF visibleArea READ visibleArea WRITE setVisibleArea NOTIFY visibleAreaChanged) //flickable pixels contentX,Y,w,h
  Q_PROPERTY(QPointF center READ center WRITE setCenter NOTIFY centerChanged) //0..1
  Q_PROPERTY(QPoint mousePos READ mousePos WRITE setMousePos NOTIFY mousePosChanged) //viewport coordinates
  Q_PROPERTY(QPointF mouse READ mouse NOTIFY mouseChanged) //0..1
  Q_PROPERTY(QPointF mouseLL READ mouseLL NOTIFY mouseLLChanged) //updated by mousePos
  Q_PROPERTY(int level READ level WRITE setLevel NOTIFY levelChanged)
  Q_PROPERTY(int levelTiles READ levelTiles NOTIFY levelTilesChanged) //current level number of tiles
  Q_PROPERTY(int maxLevel READ maxLevel NOTIFY neverChanged)
  Q_PROPERTY(int maxTiles READ maxTiles NOTIFY neverChanged)
  Q_PROPERTY(int tileSize READ tileSize NOTIFY tileSizeChanged)
  Q_PROPERTY(QPoint shift READ shift WRITE setShift NOTIFY shiftChanged) //[tiles] flick view optimizations
  Q_PROPERTY(qreal mapScaleFactor READ mapScaleFactor WRITE setMapScaleFactor NOTIFY mapScaleFactorChanged)
  Q_PROPERTY(qreal itemScaleFactor READ itemScaleFactor WRITE setItemScaleFactor NOTIFY itemScaleFactorChanged)

  QRectF visibleArea() const;
  QPointF center() const;
  QPoint mousePos() const;
  QPointF mouse() const;
  QPointF mouseLL() const;
  int level() const;
  int levelTiles() const;
  int maxLevel() const;
  int maxTiles() const;
  int tileSize() const;
  QPoint shift() const;
  qreal mapScaleFactor() const;
  qreal itemScaleFactor() const;
private:
  QRectF m_visibleArea;
  QPointF m_center;
  QPoint m_mousePos;
  QPointF m_mouse;
  QPointF m_mouseLL;
  int m_level;
  int m_levelTiles;
  int m_maxLevel;
  int m_maxTiles;
  int m_tileSize;
  QPoint m_shift;
  qreal m_mapScaleFactor;
  qreal m_itemScaleFactor;
public slots:
  void setVisibleArea(const QRectF &v);
  void setCenter(QPointF v);
  void setMousePos(QPoint v);
  void setLevel(int v);
  void setShift(QPoint v);
  void setMapScaleFactor(qreal v);
  void setItemScaleFactor(qreal v);
signals:
  void visibleAreaChanged();
  void centerChanged();
  void mousePosChanged();
  void mouseChanged();
  void mouseLLChanged();
  void levelChanged();
  void levelTilesChanged();
  void neverChanged();
  void tileSizeChanged(int);
  void shiftChanged();
  void mapScaleFactorChanged();
  void itemScaleFactorChanged();
};
//=============================================================================
#endif // QMLMAP_H
