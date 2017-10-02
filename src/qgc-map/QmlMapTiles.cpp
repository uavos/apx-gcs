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
#include "QmlMapTiles.h"
#include <QtGlobal>
//=============================================================================
QmlMapTiles::QmlMapTiles(QQuickItem *parent)
  : QQuickItem(parent),
    m_provider(NULL),
    m_downloadCnt(0)
{
  setFlag(ItemHasContents, true);
  //qDebug()<<this;
  //update();

  //connect(&tileLoader,&QmlMapTileLoader::downloadTile,&downloader,&QmlMapTileDownloader::downloadTile);
  //connect(&downloader,&QmlMapTileDownloader::tileDownloaded,this,&QmlMapTiles::tileDownloaded);

  //tile loader thread
  connect(&tileLoader,&QThread::finished,&tileLoader,&QObject::deleteLater);
  connect(this,&QmlMapTiles::loadTile,&tileLoader,&QmlMapTileLoader::loadTile,Qt::QueuedConnection);
  connect(this,&QmlMapTiles::loadTileCancel,&tileLoader,&QmlMapTileLoader::loadTileCancel,Qt::QueuedConnection);
  connect(&tileLoader,&QmlMapTileLoader::tileLoaded,this,&QmlMapTiles::tileLoaded,Qt::QueuedConnection);
  tileLoader.start(QThread::HighPriority);

  connect(&tileLoader.downloader,&QmlMapTileDownloader::progress,this,&QmlMapTiles::setDownloadCnt,Qt::QueuedConnection);
}
QmlMapTiles::~QmlMapTiles()
{
  tileLoader.quit();
  tileLoader.wait();
}
//=============================================================================
QSGNode *QmlMapTiles::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
  if(!node){
    node = new QSGNode;
    node->setFlag(QSGNode::OwnedByParent);
  }
  arrangeTiles(node);
  return node;
}
//=============================================================================
void QmlMapTiles::arrangeTiles(QSGNode *node)
{
  if(!m_provider)return;
  int sz=m_provider->tileSize();
  QRect r=m_visibleArea.toRect();
  r=QRect(r.topLeft()/sz,r.size()/sz);
  r+=QMargins(2,2,2,2);
  r.translate(m_provider->shift());
  int levelTiles=m_provider->levelTiles();
  //qDebug()<<r;
  r=r.intersected(QRect(0,0,levelTiles,levelTiles));
  //qDebug()<<r;
  if(r.size().isNull())return;
  if(visibleXY==r && loadedImages.isEmpty())return;
  visibleXY=r;
  //fill visible area with tile nodes
  QList<quint64> tlist=tiles.keys();
  //qDebug()<<r;
  int level=m_provider->level();
  for(uint x=r.x();x<=(uint)r.right();x++){
    for(uint y=r.y();y<=(uint)r.bottom();y++){
      quint64 uid=QmlMapTileLoader::uid(level,x,y);
      tlist.removeOne(uid);
      if(tiles.contains(uid)){
        QSGSimpleTextureNode *tile=tiles.value(uid);
        updateTileRect(tile,uid);
        if(!loadedImages.contains(uid)) continue;
        //image of a tile was updated (downloaded)
        if(tile){
          node->removeChildNode(tile);
          delete tile;
        }
        tiles.remove(uid);
      }
      if(!createTile(node,uid))emit loadTile(uid);
    }
  }
  loadedImages.clear();
  //clean unnecessary tiles
  foreach(quint64 uid,tlist){
    emit loadTileCancel(uid);
    QSGSimpleTextureNode *tile=tiles.value(uid);
    if(tile){
      //qDebug()<<"deleted"<<node;
      node->removeChildNode(tile);
      //delete tile->texture();
      delete tile;
    }
    tiles.remove(uid);
  }
}
//=============================================================================
bool QmlMapTiles::createTile(QSGNode *node,quint64 uid)
{
  if(!loadedImages.contains(uid))return false;
  //uint nx=QmlMapTileLoader::x(uid);
  //uint ny=QmlMapTileLoader::y(uid);
  //int sz=m_provider->tileSize();
  QSGSimpleTextureNode *tile=new QSGSimpleTextureNode();
  tile->setFlag(QSGNode::OwnedByParent);
#if QT_VERSION >= 0x050400
  tile->setOwnsTexture(true);
#endif
  //QPoint shift(m_provider->shift());
  //QRect r((nx-shift.x())*sz,(ny-shift.y())*sz,sz,sz);
  //tile->setRect(r);
  updateTileRect(tile,uid);
  tile->setTexture(window()->createTextureFromImage(loadedImages.value(uid)));
  tiles.insert(uid,tile);
  node->appendChildNode(tile);
  //qDebug()<<"createTile"<<tile;
  return true;
}
//=============================================================================
void QmlMapTiles::updateTileRect(QSGSimpleTextureNode *tile,quint64 uid)
{
  uint nx=QmlMapTileLoader::x(uid);
  uint ny=QmlMapTileLoader::y(uid);
  int sz=m_provider->tileSize();
  QPoint shift(m_provider->shift());
  QRect r((nx-shift.x())*sz,(ny-shift.y())*sz,sz,sz);
  if(tile->rect().toRect()==r)return;
  tile->setRect(r);
}
//=============================================================================
void QmlMapTiles::tileLoaded(quint64 uid, const QImage &image)
{
  //qDebug()<<"tileLoaded"<<uid<<image;
  loadedImages.insert(uid,image);
  update();
}
//=============================================================================
void QmlMapTiles::tileDownloaded(quint64 uid,const QImage &image)
{
  //qDebug()<<"tileDownloaded"<<uid<<image;

  tileLoaded(uid,image);
}
//=============================================================================
void QmlMapTiles::updateVisibleArea()
{
  setVisibleArea(m_provider->visibleArea());
}
void QmlMapTiles::updateShift()
{
  visibleXY=QRect(); //invalidate
  update();
}
//=============================================================================
//=============================================================================
QmlMap * QmlMapTiles::provider() const
{
  return m_provider;
}
void QmlMapTiles::setProvider(QmlMap *v)
{
  if(m_provider==v)return;
  m_provider=v;
  emit providerChanged();
  connect(m_provider,&QmlMap::visibleAreaChanged,this,&QmlMapTiles::updateVisibleArea);
  connect(m_provider,&QmlMap::shiftChanged,this,&QmlMapTiles::updateShift);
  connect(m_provider,&QmlMap::tileSizeChanged,&tileLoader,&QmlMapTileLoader::setTileSize);
  tileLoader.setTileSize(m_provider->tileSize());
  updateVisibleArea();
  update();
}
QRectF QmlMapTiles::visibleArea() const
{
  return m_visibleArea;
}
void QmlMapTiles::setVisibleArea(const QRectF &v)
{
  if(m_visibleArea==v)return;
  m_visibleArea=v;
  //qDebug()<<v;
  emit visibleAreaChanged();
  update();
}
int QmlMapTiles::downloadCnt()
{
  return m_downloadCnt;
}
void QmlMapTiles::setDownloadCnt(int v)
{
  if(m_downloadCnt==v)return;
  m_downloadCnt=v;
  emit downloadCntChanged();
}
//=============================================================================
//=============================================================================
void QmlMapTiles::deepDownload(void)
{
  if(!m_provider)return;
  //qDebug()<<"deepDownload...";
  foreach(quint64 uid,tiles.keys()){
    deepDownload(uid);
  }
}
//=============================================================================
void QmlMapTiles::deepDownload(quint64 uid)
{
  quint32 level=QmlMapTileLoader::level(uid);
  if(level>=m_provider->maxLevel() || (level>m_provider->level()+2))return;
  quint32 x=QmlMapTileLoader::x(uid);
  quint32 y=QmlMapTileLoader::y(uid);
  if(!QFileInfo::exists(QmlMapTileLoader::fileName(uid))){
    //qDebug()<<level<<x<<y;
    tileLoader.downloader.downloadTile(uid);
  }
  //descend deeper
  level++;
  x*=2;
  y*=2;
  deepDownload(QmlMapTileLoader::uid(level,x,y));
  deepDownload(QmlMapTileLoader::uid(level,x+1,y));
  deepDownload(QmlMapTileLoader::uid(level,x,y+1));
  deepDownload(QmlMapTileLoader::uid(level,x+1,y+1));
}
//=============================================================================
//=============================================================================
