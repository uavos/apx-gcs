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
#ifndef QmlMapTileDownloader_H
#define QmlMapTileDownloader_H
#include <QtCore>
#include <QtNetwork>
#include <QtQuick>
//=============================================================================
class QmlMapTileDownloader : public QObject
{
  Q_OBJECT
public:
  QmlMapTileDownloader(QObject *parent=0);
private:
  QNetworkRequest request;
  QNetworkAccessManager net;
  QMap<QNetworkReply*,quint64> reqMap;
  QList<quint64> downloadedUids;
  void getSecGoogleWords(const int x, const int y,  QString &sec1, QString &sec2) const;
private slots:
  void replyFinished(QNetworkReply*);
public slots:
  void abort();
  void downloadTile(quint64 uid);
signals:
  void tileDownloaded(quint64 uid,const QImage);
  void progress(int);
};
//=============================================================================
#endif
