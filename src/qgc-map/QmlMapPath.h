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
#ifndef QmlMapPath_H
#define QmlMapPath_H
#include <QtCore>
#include <QtQuick/QQuickItem>
#include "MissionPath.h"
#include "QmlMap.h"
//=============================================================================
class QmlMapPath : public QQuickItem
{
  Q_OBJECT
public:
  QmlMapPath(QQuickItem *parent = 0);
protected:
  QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

private:
  QPolygonF points;
  QSGFlatColorMaterial *material;
private slots:
  void updatePoints();

  //PROPERTIES
public:
  Q_PROPERTY(MissionPath * path READ path WRITE setPath NOTIFY pathChanged)
  Q_PROPERTY(QmlMap * provider READ provider WRITE setProvider NOTIFY pathChanged)
  Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QPoint shift READ shift WRITE setShift NOTIFY shiftChanged)

  MissionPath * path();
  void setPath(MissionPath *v);
  QmlMap * provider();
  void setProvider(QmlMap *v);

  int lineWidth();
  void setLineWidth(int v);
  QColor color();
  void setColor(QColor v);
  QPoint shift();
  void setShift(QPoint v);
private:
  MissionPath *m_path;
  QmlMap *m_provider;
  int m_lineWidth;
  QColor m_color;
  QPoint m_shift;
signals:
  void pathChanged();
  void lineWidthChanged();
  void colorChanged();
  void shiftChanged();
};
//=============================================================================
#endif // MAPTILE_H
