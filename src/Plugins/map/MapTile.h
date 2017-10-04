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
#ifndef MAPTILE_H
#define MAPTILE_H
#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsItem>
class MapTiles;
//=============================================================================
class MapTile : public QObject,public QGraphicsItem
{
  Q_OBJECT
  Q_INTERFACES(QGraphicsItem)
public:
  MapTile(MapTiles *tiles);
  enum {Type=QGraphicsItem::UserType + 100};
  int type() const{return Type;}

  void reloadImage(void);
  void upd(const QPoint &tpos,double tsize,const QString &tname);
  QString tile_name;
  int tile_level;
  int tile_size;
  QPoint tile_pos;
protected:
  MapTiles *tiles;
  QRectF boundingRect() const;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);
private:
  QPixmap pixmap;
  int imageLevel;
  uint counter;
private slots:
  void loadImage();
  void loadLabels();
  void imageLoaded(const QString & tname, const QImage &image);
public slots:
signals:
  //void needUpd(const QPoint &tpos,double tsize,const QString &tname);
  void download_request(const QString & tile_name);
};
//=============================================================================
#endif // MAPTILE_H
