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
#include "QmlView.h"
class MapTiles;
//=============================================================================
class MapTile : public QObject
{
  Q_OBJECT

public:
  MapTile(QObject *parent=0);
  ~MapTile();

//PROPERTIES
public:
  Q_PROPERTY(MapTiles * tiles READ tiles WRITE setTiles NOTIFY tilesChanged)
  Q_PROPERTY(QPoint pos READ pos WRITE setPos NOTIFY posChanged)
  Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
  Q_PROPERTY(QString fileName READ fileName NOTIFY fileNameChanged)

  MapTiles * tiles() const;
  QPoint pos() const;
  QString id() const;
  QString fileName();
private:
  MapTiles * m_tiles;
  QPoint m_pos;
  QString m_id;
private slots:
  void tile_downloaded(const QString &tile_name);

public slots:
  void setTiles(MapTiles * v);
  void setPos(QPoint v);
  void setId(QString v);
signals:
  void tilesChanged();
  void posChanged();
  void idChanged();
  void fileNameChanged();
  void imageUpdated();
};
//=============================================================================
#endif // MAPTILE_H
