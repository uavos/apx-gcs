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
#include "MapTile.h"
#include "MapTiles.h"
//=============================================================================
MapTile::MapTile(QObject *parent)
 : QObject(parent),
   m_tiles(0)
{
  //qDebug()<<"MapTile";
}
MapTile::~MapTile()
{
  //qDebug()<<"~MapTile";
  if(m_tiles){
    m_tiles->remove(this);
  }
}
//=============================================================================
MapTiles * MapTile::tiles() const
{
  return m_tiles;
}
void MapTile::setTiles(MapTiles *v)
{
  if(m_tiles)return;
  m_tiles=v;
  m_tiles->append(this);
  connect(m_tiles,SIGNAL(downloaded(QString)),this,SLOT(tile_downloaded(QString)));
  //qDebug()<<"MapTiles"<<v;
  emit tilesChanged();
}
//=============================================================================
QPoint MapTile::pos() const
{
  return m_pos;
}
void MapTile::setPos(QPoint v)
{
  if(m_pos==v)return;
  m_pos=v;
  emit posChanged();
}
//=============================================================================
QString MapTile::id() const
{
  return m_id;
}
void MapTile::setId(QString v)
{
  if(m_id==v)return;
  if(!v.isEmpty()){
    const QStringList &st=v.split('_');
    if(st.size()!=3){
      v.clear();
    }else{
      setPos(QPoint(st.at(1).toInt(),st.at(2).toInt()));
    }
  }
  m_id=v;
  emit fileNameChanged();
  emit idChanged();
  emit imageUpdated();
}
//=============================================================================
QString MapTile::fileName()
{
  if(m_id.isEmpty())return QString();
  QString fname=MapTiles::pathMaps.filePath(m_id+".jpg");
  if(QFile::exists(fname)) return fname;
  //if(!m_id.contains('-'))emit download_request(m_id);
  return QString();
}
//=============================================================================
void MapTile::tile_downloaded(const QString &tile_name)
{
  if(tile_name!=m_id)return;
  //qDebug()<<"tile_downloaded"<<tile_name;
  if(fileName().isEmpty())return;
  emit imageUpdated();
}
//=============================================================================
