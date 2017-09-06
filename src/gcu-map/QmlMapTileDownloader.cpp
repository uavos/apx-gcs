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
#include "QmlMapTileDownloader.h"
#include "QmlMapTileLoader.h"
//=============================================================================
#define Random(low,high)  ((int)(low + qrand() % (high - low)))
//=============================================================================
QmlMapTileDownloader::QmlMapTileDownloader(QObject *parent)
 : QObject(parent)
{
  //HTTP Google req
  connect(&net,SIGNAL(finished(QNetworkReply*)),SLOT(replyFinished(QNetworkReply*)));
  //set proxy if any
  QString px=QSettings().value("proxy").toString();
  if(!px.size()) QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
  while(px.size()){
    QString user,pass,name,port;
    if(px.contains('@')){
      QStringList up=px.split('@').at(0).split(':');
      if(up.size()!=2)break;
      user=up.at(0);
      pass=up.at(1);
      px.remove(0,px.indexOf('@')+1);
    }
    if(!px.size())break;
    QStringList np=px.split(':',QString::SkipEmptyParts);
    name=np.at(0);
    if(np.size()==2)port=np.at(1);
    int iport=port.toUInt();
    if(!iport)iport=8080;
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy,name,iport,user,pass));
    break;
  }
  net.setProxy(QNetworkProxy::applicationProxy());
  //load cookies file
  request.setRawHeader("User-Agent",QString("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:%1.0) Gecko/%2%3%4 Firefox/%5.0.%6").arg(QString::number(Random(3,14)), QString::number(Random(QDate().currentDate().year() - 4, QDate().currentDate().year())), QString::number(Random(11,12)), QString::number(Random(10,30)), QString::number(Random(3,14)), QString::number(Random(1,10))).toUtf8());
  request.setRawHeader("Accept","*/*");
  request.setRawHeader("Referrer", "http://maps.google.com/");
}
//=============================================================================
void QmlMapTileDownloader::downloadTile(quint64 uid)
{
  //qDebug()<<"download_tile"<<uid;
  int level=QmlMapTileLoader::level(uid);
  int x=QmlMapTileLoader::x(uid);
  int y=QmlMapTileLoader::y(uid);
  QString sec1 = ""; // after &x=...
  QString sec2 = ""; // after &zoom=...
  getSecGoogleWords(x,y,sec1,sec2);
  //QString fname=pathMaps.filePath(uid+".jpg");//(force||QImage(fname).isNull()) &&
  if((!reqMap.values().contains(uid)) && (!downloadedUids.contains(uid))){
    //QString server = "khms";
    //QString req = "kh";
    //QString url=QString("https://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(getServerNum(x,y, 4)).arg(req).arg(googleMapsVersionSat).arg("en").arg(x).arg(sec1).arg(y).arg(level).arg(sec2);
    QString url=QString("https://khm.google.com/vt/lbw/lyrs=y&x=%1&y=%2&z=%3").arg(x).arg(y).arg(level);
    request.setUrl(QUrl(url));
    reqMap.insert(net.get(request),uid);
    emit progress(reqMap.size());
  }
}
//=============================================================================
void QmlMapTileDownloader::getSecGoogleWords(const int x, const int y,  QString &sec1, QString &sec2) const
{
  const QString sGalileoS="Galileo";
  sec1 = ""; // after &x=...
  sec2 = ""; // after &zoom=...
  int seclen = ((x * 3) + y) % 8;
  sec2 = sGalileoS.left(seclen);
  if(y >= 10000 && y < 100000) sec1 = "&s=";
}
//=============================================================================
void QmlMapTileDownloader::abort()
{
  downloadedUids.clear();
  foreach(QNetworkReply* reply,reqMap.keys()){
    if(reply)reply->abort();
  }
  reqMap.clear();
  emit progress(reqMap.size());
  qDebug("%s",tr("Download aborted.").toUtf8().data());
}
//=============================================================================
void QmlMapTileDownloader::replyFinished(QNetworkReply* reply)
{
  quint64 uid=reqMap.value(reply);
  reqMap.remove(reply);
  emit progress(reqMap.size());
  if(!reply)return;
  int err=reply->error();
  QByteArray ba=reply->readAll();
  reply->deleteLater();
  if(uid==0)return;

  if(err==QNetworkReply::OperationCanceledError)return;
  if(err) {
    if(err!=203 && downloadedUids.size())qWarning("%s (%u).\n",tr("Error downloading map").toUtf8().data(),err);
    //abort();
    return;
  }
  QImage image;
  image.loadFromData(ba);
  if(image.isNull()||image.size()!=QSize(256,256)){
    qWarning("Error downloading map (not an image)");
    abort();
    return;
  }

  QString fname=QmlMapTileLoader::pathMaps.filePath(QString("%1_%2_%3.jpg").arg(QmlMapTileLoader::level(uid)).arg(QmlMapTileLoader::x(uid)).arg(QmlMapTileLoader::y(uid)));
  QFile f(fname);
  f.open(QIODevice::WriteOnly);
  f.write(ba);
  f.flush();
  f.close();

  downloadedUids.append(uid);
  if(downloadedUids.size()>1000)
    downloadedUids.takeFirst();
  emit tileDownloaded(uid,image);
  //qDebug()<<"downloaded"<<uid;
}
//=============================================================================
