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
#ifndef QmlMapTileLoader_H
#define QmlMapTileLoader_H
#include <QtCore>
#include "QmlMapTileDownloader.h"
//=============================================================================
class QmlMapTileLoader : public QThread
{
  Q_OBJECT
public:
  QmlMapTileLoader();

  static QDir pathMaps;

  static inline quint64 uid(int level,int x, int y)
  { return ((quint64)level<<48)|((quint64)x<<24)|y; }
  static inline quint32 level(quint64 uid)
  { return uid>>48; }
  static inline quint32 x(quint64 uid)
  { return (uid>>24)&((1<<24)-1); }
  static inline quint32 y(quint64 uid)
  { return uid&((1<<24)-1); }
  static inline QString bname(quint64 uid)
  { return QString("%1_%2_%3").arg(level(uid)).arg(x(uid)).arg(y(uid)); }
  static inline QString fileName(quint64 uid)
  { return pathMaps.filePath(bname(uid)+".jpg"); }

  QmlMapTileDownloader downloader;
protected:
  void run();
private:
  QPixmap pixmap(quint64 uid);
  QList<quint64> loadList;
  QHash<quint64,QImage> loadedImages;
  QList<quint64> loadedImagesCacheIdx;
  QMutex reqMutex;
  //QMutex dataMutex;
  QWaitCondition waitCondition;
  int m_tileSize;
public slots:
  void loadTile(quint64 uid);
  void loadTileCancel(quint64 uid);
  void tileDownloaded(quint64 uid,const QImage &image);
  void setTileSize(int v);
signals:
  void downloadTile(quint64 uid);
  void tileLoaded(quint64 uid,const QImage &image);
};
//=============================================================================
#endif
