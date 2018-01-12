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
//=============================================================================
class TileLoader : public QThread
{
  Q_OBJECT
  Q_ENUMS(MapID)

  Q_PROPERTY(int requestCount READ requestCount NOTIFY requestCountChanged)

public:
  TileLoader(QObject *parent = 0);

  enum MapID
  {
    GoogleHybrid,
    GoogleSatellite,
  };
  Q_ENUM(MapID)


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

protected:
  void run() override;
private:
  //queue
  QList<quint64> queue;
  QList<quint64> queueCancelled;
  QHash<quint64,QByteArray> queueDownloaded;

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
  QHash<QNetworkReply*,quint64> reqMap;
  QList<quint64> pendingDownloads;


  int getServerNum(int x, int y, int max);
  void getSecGoogleWords(int x, int y, QString& sec1, QString& sec2);
  bool checkGoogleVersion(QNetworkRequest *request);

  bool checkImage(const QByteArray &data);


  //int m_requestCount;
  int requestCount(){return reqMap.size();}
  //void incRequestCount(){m_requestCount++;emit requestCountChanged();}
  //void decRequestCount(){m_requestCount--;emit requestCountChanged();}
private slots:
  void _fwd_tileLoaded(quint64 uid, QByteArray data);
  void download(quint64 uid);

  void networkReplyError(QNetworkReply::NetworkError error);
  void networkReplyFinished();
  void versionReplyFinished();


public slots:
  void loadTile(quint64 uid);
  void loadCancel(quint64 uid);
  void abort();

signals:
  void tileLoaded(quint64 uid, QByteArray data);
  void tileError(quint64 uid, QString errorString);

  //thread forward
  void _thr_tileLoaded(quint64 uid, QByteArray data);
  void _thr_download(quint64 uid);

  void requestCountChanged();



};
//=============================================================================
#endif
