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
#ifndef QmlMapTiles_H
#define QmlMapTiles_H
#include <QtCore>
#include <QtQuick>
#include "QmlMap.h"
#include "QmlMapTileLoader.h"
//=============================================================================
class QmlMapTiles : public QQuickItem
{
  Q_OBJECT
public:
  QmlMapTiles(QQuickItem *parent=0);
  ~QmlMapTiles();

  Q_INVOKABLE void deepDownload(void);

protected:
  QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *);
private:
  void arrangeTiles(QSGNode *node);
  bool createTile(QSGNode *node,quint64 uid);
  void updateTileRect(QSGSimpleTextureNode *tile,quint64 uid);
  QRect visibleXY;
  QHash<quint64,QSGSimpleTextureNode*> tiles;
  QHash<quint64,QImage> loadedImages;
  QList<quint64> downloadedImages;

  void deepDownload(quint64 uid);
  //async image loader
private:
  QmlMapTileLoader tileLoader;
  //QmlMapTileDownloader downloader;
signals:
  void loadTile(quint64 uid);
  void loadTileCancel(quint64 uid);
private slots:
  void tileLoaded(quint64 uid,const QImage &image);
  void tileDownloaded(quint64 uid,const QImage &image);
  void updateVisibleArea();
  void updateShift();


  //PROPERTIES
public:
  Q_PROPERTY(QmlMap * provider READ provider WRITE setProvider NOTIFY providerChanged)
  Q_PROPERTY(QRectF visibleArea READ visibleArea WRITE setVisibleArea NOTIFY visibleAreaChanged)
  Q_PROPERTY(int downloadCnt READ downloadCnt WRITE setDownloadCnt NOTIFY downloadCntChanged)

  QmlMap * provider() const;
  void setProvider(QmlMap *v);
  QRectF visibleArea() const;
  void setVisibleArea(const QRectF &v);
  int downloadCnt();
  void setDownloadCnt(int v);
private:
  QmlMap * m_provider;
  QRectF m_visibleArea;
  int m_downloadCnt;
signals:
  void providerChanged();
  void visibleAreaChanged();
  void downloadCntChanged();
};
//=============================================================================
#endif
