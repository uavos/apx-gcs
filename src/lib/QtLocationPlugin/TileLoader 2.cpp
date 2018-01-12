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
#include "TileLoader.h"
#include "GeoPlugin.h"
//=============================================================================
#define Random(low,high)  ((int)(low + qrand() % (high - low)))
//=============================================================================
TileLoader::TileLoader(QObject *parent)
 : QObject(parent)
{
  TileLoaderWorker *worker = new TileLoaderWorker;
  worker->moveToThread(&workerThread);
  connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
  connect(this, &TileLoader::w_loadTile, worker, &TileLoaderWorker::loadTile);
  connect(this, &TileLoader::w_loadCancel, worker, &TileLoaderWorker::loadCancel);
  connect(worker, &TileLoaderWorker::tileLoaded, this, &TileLoader::w_tileLoaded);
  workerThread.start();
}
TileLoader::~TileLoader()
{
  workerThread.quit();
  workerThread.wait();
}
void TileLoader::loadTile(quint64 uid)
{
  emit w_loadTile(uid);
}
void TileLoader::loadCancel(quint64 uid)
{
  emit w_loadCancel(uid);
}
void TileLoader::w_tileLoaded(quint64 uid, QByteArray data)
{
  emit tileLoaded(uid,data);
}
//=============================================================================
//=============================================================================
TileLoaderWorker::TileLoaderWorker(QObject *parent)
 : QObject(parent)
{
  _db=new MapsDB(this,QLatin1String("GCSTileLoaderWorkerSession"));

  net=new QNetworkAccessManager(this);

  userAgent=QString("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:%1.0) Gecko/%2%3%4 Firefox/%5.0.%6").arg(QString::number(Random(3,14)), QString::number(Random(QDate().currentDate().year() - 4, QDate().currentDate().year())), QString::number(Random(11,12)), QString::number(Random(10,30)), QString::number(Random(3,14)), QString::number(Random(1,10))).toUtf8();
  /*QStringList langs = QLocale::system().uiLanguages();
  if (langs.length()>0){
    language=langs[0];
    qDebug()<<language;
  }*/
  language="en-US";

  connect(net,&QNetworkAccessManager::finished,this,[=](){qDebug()<<"finished"<<m_requestCount;});
}
//=============================================================================
void TileLoaderWorker::loadTile(quint64 uid)
{
  QByteArray data=_db->readTile(TileLoader::type(uid),TileLoader::dbHash(uid));
  if(queueCancelled.contains(uid)){
    queueCancelled.removeOne(uid);
  }else{
    emit tileLoaded(uid,data);
  }

  /*queueMutex.lock();
  queueCancelled.removeOne(uid);
  if(queue.contains(uid)){
    queueMutex.unlock();
    return;
  }
  queue.append(uid);
  queueMutex.unlock();
  waitCondition.wakeAll();*/
}
void TileLoaderWorker::loadCancel(quint64 uid)
{
  //move to the bottom to load later
  /*queueMutex.lock();
  if(!queue.contains(uid)){
    queueMutex.unlock();
    return;
  }
  queue.removeOne(uid);
  queueCancelled.append(uid);
  queueMutex.unlock();*/
  queueCancelled.append(uid);
  qDebug()<<"Cancel: "<<uid;
}
//=============================================================================
/*void TileLoaderWorker::run()
{
  forever{
    if(QThread::currentThread()->isInterruptionRequested())return;
    //usleep(1000);
    quint64 uid;
    bool bCancelled=false;
    queueMutex.lock();
    if(queue.isEmpty()){
      if(queueCancelled.isEmpty()){
        queueMutex.unlock();
        reqMutex.lock();
        waitCondition.wait(&reqMutex);
        reqMutex.unlock();
        queueMutex.lock();
        uid=queue.takeFirst();
      }else{
        uid=queueCancelled.takeFirst();
        bCancelled=true;
      }
    }else uid=queue.takeFirst();
    queueMutex.unlock();
    //load tile uid

    QByteArray data=_db->readTile(type(uid),dbHash(uid));
    if(data.isEmpty()){

    }

    //signal
    if(!bCancelled){
      emit _thr_tileLoaded(uid,data);
    }
  }
}*/
//=============================================================================
void TileLoaderWorker::download(quint64 uid)
{
  //download
  QNetworkRequest request;
  GeoPlugin::MapID type=(GeoPlugin::MapID)TileLoader::type(uid);
  int x=TileLoader::x(uid);
  int y=TileLoader::y(uid);
  int zoom=TileLoader::level(uid);
  bool bVersionRequest=false;
  //qDebug()<<spec<<type;
  switch(type) {
    default: break;
    case GeoPlugin::GoogleSatellite: {
      // http://mt1.google.com/vt/lyrs=s
      if(!checkGoogleVersion(&request)){
        if(request.url().isEmpty())return;
        bVersionRequest=true;
      }else{
        QString server  = "khm";
        QString req     = "kh";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        getSecGoogleWords(x, y, sec1, sec2);
        request.setUrl(QUrl(QString("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(getServerNum(x, y, 4)).arg(req).arg(versionGoogleMaps).arg(language).arg(x).arg(sec1).arg(y).arg(zoom).arg(sec2)));
        request.setRawHeader("Referrer", "https://www.google.com/maps/preview");
      }
    }break;
    case GeoPlugin::GoogleHybrid: {
      if(!checkGoogleVersion(&request)){
        if(request.url().isEmpty())return;
        bVersionRequest=true;
      }else{
        request.setUrl(QUrl(QString("https://khm.google.com/vt/lbw/lyrs=y&x=%1&y=%2&z=%3").arg(x).arg(y).arg(zoom)));
        request.setRawHeader("Referrer", "http://maps.google.com/");
      }
    }break;
  }
  //begin download
  QNetworkReply *reply = downloadRequest(&request);
  //qDebug()<<url;
  connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),this, &TileLoaderWorker::networkReplyError);
  if(bVersionRequest){
    connect(reply,&QNetworkReply::finished, this, &TileLoaderWorker::versionReplyFinished);
    pendingDownloads.append(uid);
  }else{
    connect(reply,&QNetworkReply::finished, this, &TileLoaderWorker::networkReplyFinished);
    downloads.insert(reply,uid);
  }
  //timeout timer
  //timeoutTimer.start();
}
//=============================================================================
QNetworkReply * TileLoaderWorker::downloadRequest(QNetworkRequest *request)
{
#if !defined(__mobile__)
  QNetworkProxy proxy = net->proxy();
  QNetworkProxy tProxy;
  tProxy.setType(QNetworkProxy::DefaultProxy);
  net->setProxy(tProxy);
#endif

  QSslConfiguration ssl = request->sslConfiguration();
  ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
  request->setSslConfiguration(ssl);

  request->setRawHeader("Accept", "*/*");
  request->setRawHeader("User-Agent", userAgent);

  QNetworkReply *reply=net->get(*request);
  incRequestCount();

#if !defined(__mobile__)
  net->setProxy(proxy);
#endif
  return reply;
}
//=============================================================================
bool TileLoaderWorker::checkGoogleVersion(QNetworkRequest *request)
{
  if(!versionGoogleMaps.isEmpty()) return true;
  if(versionGoogleMaps=="?") return false;
  versionGoogleMaps="?";
  request->setUrl(QUrl("http://maps.google.com/maps/api/js?v=3.2&sensor=false"));
  request->setRawHeader("Referrer", "https://www.google.com/maps/preview");
  return false;
}
//=============================================================================
void TileLoaderWorker::getSecGoogleWords(int x, int y, QString &sec1, QString &sec2)
{
  sec1 = ""; // after &x=...
  sec2 = ""; // after &zoom=...
  int seclen = ((x * 3) + y) % 8;
  sec2 = QLatin1String("Galileo").left(seclen);
  if (y >= 10000 && y < 100000) {
    sec1 = "&s=";
  }
}
//=============================================================================
int TileLoaderWorker::getServerNum(int x, int y, int max)
{
  return (x + 2 * y) % max;
}
//=============================================================================
//=============================================================================
void TileLoaderWorker::networkReplyFinished()
{
  QNetworkReply *reply=qobject_cast<QNetworkReply*>(sender());
  if(!reply)return;
  if (reply->error() != QNetworkReply::NoError) {
    return;
  }
  QByteArray data = reply->readAll();
  quint64 uid=downloads.value(reply);
  emit tileLoaded(uid,data);
}
//=============================================================================
void TileLoaderWorker::networkReplyError(QNetworkReply::NetworkError error)
{
  QNetworkReply *reply=qobject_cast<QNetworkReply*>(sender());
  if(!reply)return;
  qWarning() << "Network error:" << reply->errorString();
  /*if(error != QNetworkReply::OperationCanceledError) {
    qWarning() << "Network error:" << reply->errorString();
    setError(QGeoTiledMapReply::CommunicationError, reply->errorString());
    //retryTimer.start();
  }else{
    setFinished(true);
  }*/
}
//=============================================================================
void TileLoaderWorker::versionReplyFinished()
{
  QNetworkReply *reply=qobject_cast<QNetworkReply*>(sender());
  if(!reply)return;
  reply->deleteLater();
  if (reply->error() != QNetworkReply::NoError) {
    qDebug() << "Error collecting Google maps version info";
    return;
  }
  QString html = QString(reply->readAll());

#if defined(DEBUG_GOOGLE_MAPS)
  QString filename = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  filename += "/google.output";
  QFile file(filename);
  if (file.open(QIODevice::ReadWrite))
  {
    QTextStream stream( &file );
    stream << html << endl;
  }
#endif

  QRegExp reg("\"*https?://mt\\D?\\d..*/vt\\?lyrs=m@(\\d*)", Qt::CaseInsensitive);
  if (reg.indexIn(html) != -1) {
    QStringList gc = reg.capturedTexts();
    //versions[GoogleMap] = QString("m@%1").arg(gc[1]);
  }
  reg = QRegExp("\"*https?://khm\\D?\\d.googleapis.com/kh\\?v=(\\d*)", Qt::CaseInsensitive);
  if (reg.indexIn(html) != -1) {
    QStringList gc = reg.capturedTexts();
    versionGoogleMaps=gc[1];
  }
  reg = QRegExp("\"*https?://mt\\D?\\d..*/vt\\?lyrs=t@(\\d*),r@(\\d*)", Qt::CaseInsensitive);
  if (reg.indexIn(html) != -1) {
    QStringList gc = reg.capturedTexts();
    //_versionGoogleTerrain = QString("t@%1,r@%2").arg(gc[1]).arg(gc[2]);
  }
}
//=============================================================================
