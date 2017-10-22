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
#include "MapView.h"
#include <QStyleOptionGraphicsItem>
#include "AppDirs.h"
//=============================================================================
MapTile::MapTile(MapTiles *tiles)
  :QObject(),QGraphicsItem(),tiles(tiles),counter(0)
{
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);
  //setCacheMode(QGraphicsItem::NoCache);
  //setCacheMode(QGraphicsItem::ItemCoordinateCache);
  setZValue(-100000);

  //connect(tiles,SIGNAL(loaded(const QString &, const QImage &)),this,SLOT(imageLoaded(const QString &, const QImage &)));
}
//=============================================================================
void MapTile::upd(const QPoint &tpos,double tsize,const QString &tname)
{
  //emit needUpd(tpos,tsize,tname);
  if(tile_name!=tname){
    pixmap=QPixmap();
    tile_name=tname;
    tile_level=tname.left(tname.indexOf('_')).toUInt();
    reloadImage();
    //tiles->load_tile(tile_name);
  }else return;
  prepareGeometryChange();
  tile_size=tsize;
  tile_pos=tpos;
  if(pos()!=tile_pos)setPos(tile_pos);
}
void MapTile::imageLoaded(const QString &tname, const QImage &image)
{
  if(tname!=tile_name)return;
  pixmap.convertFromImage(image);
  update();
}
//=============================================================================
void MapTile::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);
  Q_UNUSED(option);
  //painter->setClipRect(option->exposedRect);
  painter->setRenderHint(QPainter::SmoothPixmapTransform,false);

  if (!pixmap.isNull())
    painter->drawPixmap(0,0,tile_size,tile_size,pixmap);

  /*QRectF r=boundingRect();
  double sf=tile_size/256.0;
  QPen pen(Qt::white,0,Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawRect(r);
  painter->scale(sf,sf);
  painter->drawText(QRectF(r.topLeft()/sf,r.size()/sf),Qt::AlignLeft|Qt::AlignTop,QString().sprintf("%u",counter++));
*/
  /*if(isSelected()||view->showBlockFrames){
    painter->setPen(Qt::cyan);
    painter->drawRect(boundingRect());
  }*/

  /*QRectF r=boundingRect();
  QPen pen(Qt::white,0,Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawRect(r);
  double sf=tile_size/256.0;
  painter->scale(sf,sf);
  painter->drawText(QRectF(r.topLeft()/sf,r.size()/sf),Qt::AlignLeft|Qt::AlignBottom,QString().sprintf("pos(%.1f,%.1f)\nsz(%u)\nsf[%.2f] :%u",pos().x(),pos().y(),tile_size,scale(),counter++));
  //pen.setColor
  painter->setPen(pen);
  //painter->drawRect(option->exposedRect);*/
}
//=============================================================================
void MapTile::loadImage()
{
  if(tile_name.isEmpty())return;
  imageLevel=tile_level;
  /*QStringList st=tile_name.split('_');
  QByteArray ba=tiles->getImageFromCache(MapTiles::GoogleSatellite,QPoint(st.at(1).toUInt(),st.at(2).toUInt()),st.at(0).toUInt());
  if(ba.size()){
    image.loadFromData(ba);
    return;
  }*/
  QString fname=MapTiles::pathMaps.filePath(tile_name+".jpg");
  if (QFile::exists(fname)) {
    pixmap.load(fname);
    if (pixmap.isNull()||pixmap.size()!=QSize(256,256))QFile::remove(fname);
    else{
      //test=image.toImage();
      //image=image.scaled(tile_size,tile_size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
      return;
    }
  }
  emit download_request(tile_name);
  pixmap=QPixmap();

  //look for image in reources
  fname=AppDirs::res().absoluteFilePath("maps/google-tiles/"+tile_name+".jpg");
  if(QFile::exists(fname)) {
    pixmap.load(fname);
    return;
  }

  //compose image from lower res..
  QString bname=tile_name;
  const QStringList lp=bname.split('_');
  int bX=lp.at(1).toUInt();
  int bY=lp.at(2).toUInt();
  QRectF  r(0.0,0.0,256.0,256.0);
  while (imageLevel) {
    imageLevel--;
    r.setSize(r.size()/2.0);
    r.moveTopLeft(r.topLeft()/2.0);
    //get next low res bname..
    bname=QString::number(imageLevel)+"_"+QString::number(bX/2)+"_"+QString::number(bY/2);
    double sz=128.0;
    if(bX%2)r.translate(sz,0.0);
    if(bY%2)r.translate(0.0,sz);
    bX=bX/2;
    bY=bY/2;

    QString fname=MapTiles::pathMaps.filePath(bname+".jpg");
    if (!QFile::exists(fname))continue;
    //draw image..
    QPixmap img(fname);
    if (img.isNull()) {
      QFile::remove(fname);
      continue;
    }
    pixmap=QPixmap(256,256);//,QImage::Format_RGB32
    if (!img.isNull()){
      QPainter p(&pixmap);
      p.drawPixmap(QRectF(0,0,256,256),img,r);
      QPen pen(QColor(255,0,0,100));
      pen.setCosmetic(true);
      p.setPen(pen);
      p.drawRect(pixmap.rect());
      pen=QPen(QColor(255,255,255,100));
      pen.setCosmetic(true);
      p.setPen(pen);
      p.drawText(0,12,"L:"+QString::number(imageLevel));
    }
    break;
  }
}
//=============================================================================
void MapTile::loadLabels()
{
  return;
  /*QStringList st=tile_name.split('_');
  QByteArray ba=tiles->getImageFromCache(MapTiles::GoogleLabels,QPoint(st.at(1).toUInt(),st.at(2).toUInt()),st.at(0).toUInt());
  if(ba.size()){
    imageLabels.loadFromData(ba);
    if(!image.isNull()) QPainter(&image).drawImage(0,0,imageLabels);
    return;
  }*/
  QString fname=MapTiles::pathMaps.filePath(tile_name+".png");
  if (QFile::exists(fname)) {
    QImage img=QImage(fname);
    if (img.isNull()||img.size()!=QSize(256,256)){
      QFile::remove(fname);
    }else{
      if(pixmap.isNull())return;
      QPainter(&pixmap).drawImage(0,0,img);
      return;
    }
  }
  emit download_request(tile_name);
}
//=============================================================================
void MapTile::reloadImage(void)
{
  loadImage();
  loadLabels();
}
//=============================================================================
QRectF MapTile::boundingRect() const
{
  return QRectF(0,0,tile_size,tile_size);
}
//=============================================================================
//=============================================================================
