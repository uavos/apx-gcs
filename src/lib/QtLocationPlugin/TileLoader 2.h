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
#ifndef TileLoader_H
#define TileLoader_H
#include <QtCore>
#include <MapsDB.h>
#include <QtNetwork>
class TileLoaderWorker;
//=============================================================================
class TileLoader : public QObject
{
  Q_OBJECT
public:
  TileLoader(QObject *parent = 0);
  ~TileLoader();

  static inline quint64 uid(quint8 type, quint8 level,quint32 x, quint32 y)
  { return ((quint64)type<<56)|((quint64)level<<48)|((quint64)x<<24)|(quint64)y; }
  static inline quint8 type(quint64 uid)
  { return (uid>>56)&(((quint64)1<<8)-1); }
  static inline quint8 level(quint64 uid)
  { return (uid>>48)&(((quint64)1<<8)-1); }
  static inline quint32 x(quint64 uid)
  { return (uid>>24)&(((quint64)1<<24)-1); }
  static inline quint32 y(quint64 uid)
  { return uid&(((quint64)1<<24)-1); }
  static inline quint64 dbHash(quint64 uid)
  { return uid&(((quint64)1<<56)-1); }


private:
  QThread workerThread;
private slots:
  void w_tileLoaded(quint64 uid, QByteArray data);

public slots:
  void loadTile(quint64 uid);
  void loadCancel(quint64 uid);

signals:
  void tileLoaded(quint64 uid, QByteArray data);

  void w_loadTile(quint64 uid);
  void w_loadCancel(quint64 uid);
};
//=============================================================================
class TileLoaderWorker : public QObject
{
  Q_OBJECT
public:
  TileLoaderWorker(QObject *parent = 0);


private:
  //queue
  QList<quint64> queue;
  QList<quint64> queueCancelled;

  QHash<QNetworkReply*,quint64> downloads;
  QList<quint64> pendingDownloads;

  QMutex queueMutex;
  QMutex reqMutex;
  QWaitCondition waitCondition;

  //cache
  MapsDB *_db;

  //downloader
  QNetworkAccessManager *net;
  QByteArray userAgent;
  QString language;
  QString versionGoogleMaps;
  QNetworkReply * downloadRequest(QNetworkRequest *request);
  void download(quint64 uid);

  int getServerNum(int x, int y, int max);
  void getSecGoogleWords(int x, int y, QString& sec1, QString& sec2);
  bool checkGoogleVersion(QNetworkRequest *request);



  int m_requestCount;
  void incRequestCount(){m_requestCount++;}
  void decRequestCount(){m_requestCount--;}
private slots:
  void networkReplyError(QNetworkReply::NetworkError error);
  void networkReplyFinished();
  void versionReplyFinished();


public slots:
  void loadTile(quint64 uid);
  void loadCancel(quint64 uid);

signals:
  void tileLoaded(quint64 uid, QByteArray data);
};
//=============================================================================
#endif
