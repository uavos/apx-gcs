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
#include "QmlMapTileLoader.h"
#include "QMandala.h"
//=============================================================================
QDir QmlMapTileLoader::pathMaps;
QmlMapTileLoader::QmlMapTileLoader()
 : QThread()
{
  m_tileSize=256;

  pathMaps=QDir(QMandala::Global::maps().filePath("google-tiles"));
  if(!pathMaps.exists())pathMaps.mkpath(".");

  //connect(&tileProvider,QmlMapTileProvider::
  connect(this,&QmlMapTileLoader::downloadTile,&downloader,&QmlMapTileDownloader::downloadTile,Qt::QueuedConnection);
  connect(&downloader,&QmlMapTileDownloader::tileDownloaded,this,&QmlMapTileLoader::tileDownloaded,Qt::QueuedConnection);
}
//=============================================================================
void QmlMapTileLoader::loadTile(quint64 uid)
{
  //dataMutex.lock();
  if(loadList.contains(uid)){
    //dataMutex.unlock();
    return;
  }
  loadList.append(uid);
  //dataMutex.unlock();
  waitCondition.wakeAll();
}
void QmlMapTileLoader::loadTileCancel(quint64 uid)
{
  if(loadList.contains(uid))qDebug()<<"Cancel: "<<uid;
  loadList.removeOne(uid);
}
//=============================================================================
void QmlMapTileLoader::setTileSize(int v)
{
  //dataMutex.lock();
  if(m_tileSize==v){
    //dataMutex.unlock();
    return;
  }
  m_tileSize=v;
  //qDebug()<<"setTileSize: "<<v;
  QList<quint64> klist=loadedImages.keys();
  loadedImages.clear();
  loadedImagesCacheIdx.clear();
  //dataMutex.unlock();
  //reload all
  foreach(quint64 uid,klist){
    loadTile(uid);
  }
}
//=============================================================================
void QmlMapTileLoader::run()
{
  //image file loader
  forever{
    if(QThread::currentThread()->isInterruptionRequested())return;
    //usleep(1000);
    //dataMutex.lock();
    if(loadList.isEmpty()){
      //dataMutex.unlock();
      reqMutex.lock();
      waitCondition.wait(&reqMutex);
      reqMutex.unlock();
      //dataMutex.lock();
    }
    quint64 uid=loadList.first();
    if(loadedImages.contains(uid)){
      //cached image found
      loadList.removeOne(uid);
      emit tileLoaded(uid,loadedImages.value(uid));
      //dataMutex.unlock();
      continue;
    }
    //dataMutex.unlock();
    //qDebug()<<"loader req: "<<uid;
    QPixmap pixmap=this->pixmap(uid);
    if(pixmap.isNull())continue;
    if(m_tileSize!=pixmap.size().width())pixmap=pixmap.scaled(m_tileSize,m_tileSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QImage image=pixmap.toImage();
    //dataMutex.lock();
    loadedImages.insert(uid,image);
    loadList.removeOne(uid);
    loadedImagesCacheIdx.append(uid);
    if(loadedImagesCacheIdx.size()>256){
      //qDebug()<<"cache rm: "<<loadedImagesCacheIdx.first();
      loadedImages.remove(loadedImagesCacheIdx.first());
      loadedImagesCacheIdx.removeFirst();
    }
    //dataMutex.unlock();
    emit tileLoaded(uid,image);
  }
}
//=============================================================================
void QmlMapTileLoader::tileDownloaded(quint64 uid,const QImage &image)
{
  //qDebug()<<"tileDownloaded"<<uid<<image;
  //dataMutex.lock();
  loadedImages.remove(uid);
  loadedImagesCacheIdx.removeOne(uid);
  //dataMutex.unlock();
  emit tileLoaded(uid,image);
}
//=============================================================================
//=============================================================================
QPixmap QmlMapTileLoader::pixmap(quint64 uid)
{
  int level=this->level(uid);
  int bX=this->x(uid);
  int bY=this->y(uid);
  QString fname=fileName(uid);
  QPixmap pixmap;
  bool skipDownload=true;
  while(1){
    if(level>20){
      break;
    }
    if(!QFile::exists(fname)){
      skipDownload=false;
      break;
    }
    if(QFileInfo(fname).size()==664){
      //qDebug()<<"black";
      break;
    }
    pixmap.load(fname);
    if(pixmap.isNull()||pixmap.size()!=QSize(256,256)){
      QFile::remove(fname);
      skipDownload=false; //re-download
      break;
    }
    if(level>19){
      if(pixmap.hasAlpha()||pixmap.hasAlphaChannel())
        break;
      QImage img=pixmap.toImage();
      if(img.pixel(0,0)==0xFF000000
         || img.pixel(255,0)==0xFF000000
         || img.pixel(255,255)==0xFF000000
         || img.pixel(0,255)==0xFF000000
         ){
        break;
      }
    }
    //pixmap looks good
    return pixmap;
  }
  //qDebug()<<"no pixmap"<<bname;
  pixmap=QPixmap(256,256);
  pixmap.fill(Qt::transparent);
  if(!skipDownload)emit downloadTile(uid);

  //compose image from lower res..
  int imageLevel=level;
  QRectF  r(0.0,0.0,256.0,256.0);
  while(imageLevel) {
    imageLevel--;
    r.setSize(r.size()/2.0);
    r.moveTopLeft(r.topLeft()/2.0);
    //get next low res bname..
    double sz=128.0;
    if(bX%2)r.translate(sz,0.0);
    if(bY%2)r.translate(0.0,sz);
    bX=bX/2;
    bY=bY/2;
    if(imageLevel>20)continue;
    QString fname=fileName(this->uid(imageLevel,bX,bY));
    if(!QFile::exists(fname)){
      emit downloadTile(this->uid(imageLevel,bX,bY));
      continue;
    }
    if(QFileInfo(fname).size()==664)continue;
    //draw image..
    QPixmap img(fname);
    if (img.isNull()) {
      QFile::remove(fname);
      continue;
    }
    pixmap=QPixmap(256,256);//,QImage::Format_RGB32
    if (!img.isNull()){
      QPainter p(&pixmap);
      p.setRenderHint(QPainter::SmoothPixmapTransform,true);
      p.drawPixmap(QRectF(0,0,256,256),img,r);
      if(!skipDownload){
        QPen pen(QColor(255,0,0,100));
        pen.setCosmetic(true);
        p.setPen(pen);
        p.drawRect(pixmap.rect());
        pen=QPen(QColor(255,255,255,100));
        //pen.setCosmetic(true);
        //p.setPen(pen);
        //p.drawText(pixmap.rect(),Qt::AlignTop,"L:"+QString::number(imageLevel));
      }
    }
    break;
  }
  return pixmap;
}
//=============================================================================
