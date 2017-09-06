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
#ifndef MAPTILES_H
#define MAPTILES_H
#include <QtCore>
#include <QtNetwork>
#include "MapTile.h"
class MapView;
//=============================================================================
class MapTiles : public QObject
{
  Q_OBJECT
public:
  MapTiles(MapView *view);

  QGraphicsScene *scene;
  QList<MapTile*> tiles;
  int level;

  enum _maptype {
    GoogleMap=1,
    GoogleSatellite=4,
    GoogleLabels=8,
    GoogleTerrain=16,
    GoogleHybrid=20,

    GoogleMapChina=22,
    GoogleSatelliteChina=24,
    GoogleLabelsChina=26,
    GoogleTerrainChina=28,
    GoogleHybridChina=29,

    OpenStreetMap=32,
    OpenStreetOsm=33,
    OpenStreetMapSurfer=34,
    OpenStreetMapSurferTerrain=35,

    YahooMap=64,
    YahooSatellite=128,
    YahooLabels=256,
    YahooHybrid=333,

    BingMap=444,
    BingSatellite=555,
    BingHybrid=666,

    ArcGIS_Map=777,
    ArcGIS_Satellite=788,
    ArcGIS_ShadedRelief=799,
    ArcGIS_Terrain=811,

    ArcGIS_MapsLT_Map=1000,
    ArcGIS_MapsLT_OrtoFoto=1001,
    ArcGIS_MapsLT_Map_Labels=1002,
    ArcGIS_MapsLT_Map_Hybrid=1003,

    PergoTurkeyMap = 2001,
    SigPacSpainMap = 3001,

    GoogleMapKorea=4001,
    GoogleSatelliteKorea=4002,
    GoogleLabelsKorea=4003,
    GoogleHybridKorea=4005,

    YandexMapRu = 5000
  };
  QByteArray getImageFromCache(_maptype type, QPoint pos, int zoom);

  void setScene(QGraphicsScene *scene);

  bool updateLevel(double sf);

  QString lastUrl;
  int download_cnt(void){return reqMap.size()+req_ver_queue.size();}

  static QString cookiesFileName;

  static QDir pathMaps;
  static const int maxLevel,minLevel;
  static const double MaxTiles;
  static const double MaxTiles2;
  static const int NumTiles[];
private:
  MapView *view;
  static const int BitmapSize[];
  static const int BitmapOrigo[];

  void iterateBlockNames(QString bname,QStringList &list);

  bool autoDownload;
  QNetworkRequest request;
  QNetworkAccessManager net;
  QMap<QNetworkReply*,QString> reqMap;
  QStringList req_ver_queue;
  QStringList downloaded_fnames;
  QString googleMapsVersionSat;
  QString googleMapsVersionLabels;
  void getSecGoogleWords(const int x, const int y,  QString &sec1, QString &sec2) const;
  int getServerNum(const int x, const int y, const int max) const;

  //cache
  /*QReadWriteLock lock;
  static qlonglong ConnCounter;
  QMutex Mcounter;
  QString dbfile;
  bool createEmptyDB(const QString &file);
  bool putImageToCache(const QByteArray &tile,const _maptype &type,const QPoint &pos, const int &zoom);
  void deleteOlderTiles(int const& days);*/


private slots:
  void replyFinished(QNetworkReply*);
  void download_tile(const QString & tile_name);
signals:
  void downloaded(const QString &tile_name);
public slots:
  void updateTiles();
  void setAutoDownload(bool v);
  void refresh();
  void abort();
  void downloadDeep(QString bname);
};
//=============================================================================
#endif // MAPTILES_H
