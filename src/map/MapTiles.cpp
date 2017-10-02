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
#include "MapTiles.h"
#include "MapView.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include "QMandala.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "MapView.h"
//=============================================================================
//=============================================================================
QString MapTiles::cookiesFileName="google-cookies";
#define Random(low,high)  ((int)(low + qrand() % (high - low)))
const int MapTiles::maxLevel=19;
const int MapTiles::minLevel=1;
const int MapTiles::NumTiles[]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288};
const int MapTiles::BitmapSize[]={1*256,2*256,4*256,8*256,16*256,32*256,64*256,128*256,256*256,512*256,1024*256,2048*256,4096*256,8192*256,16384*256,32768*256,65536*256,131072*256,262144*256,524288*256};
const int MapTiles::BitmapOrigo[]={1*128,2*128,4*128,8*128,16*128,32*128,64*128,128*128,256*128,512*128,1024*128,2048*128,4096*128,8192*128,16384*128,32768*128,65536*128,131072*128,262144*128,524288*128};
const double MapTiles::MaxTiles=MapTiles::NumTiles[maxLevel];
const double MapTiles::MaxTiles2=MapTiles::MaxTiles/2;
QDir MapTiles::pathMaps;
//qlonglong MapTiles::ConnCounter=0;
//=============================================================================
//=============================================================================
MapTiles::MapTiles(MapView *view)
  :QObject(),scene(NULL),view(view)
{
  pathMaps=QDir(QMandala::Global::maps().absoluteFilePath("google-tiles"));
  if(!pathMaps.exists())pathMaps.mkpath(".");
  //check default maps
  /*QDir pmLib(QMandala::Global::res());
  if(pmLib.cd("maps")){
    bool bUpd=false;
    foreach(QFileInfo fi,pmLib.entryInfoList(QDir::Files)){
      QFileInfo fiLocal(pathMaps.filePath(fi.fileName()));
      if(!fiLocal.exists() || fiLocal.lastModified()<fi.lastModified()){
          QFile::remove(fiLocal.absoluteFilePath());
          QFile::copy(fi.absoluteFilePath(),fiLocal.absoluteFilePath());
          bUpd=true;
      }
    }
    if(bUpd)qDebug("%s",tr("Default maps updated").toUtf8().data());
  }*/
  //------
  level=maxLevel-2;

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
  //cache
  /*dbfile=QMandala::Global::local.filePath("maps.qmdb");
  if(!QFileInfo(dbfile).exists())
    if(!createEmptyDB(dbfile))dbfile="";*/

  /*QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
  diskCache->setCacheDirectory(QMandala::Global::local.filePath("maps_cache"));
  net.setCache(diskCache);
  net.setNetworkAccessible(QNetworkAccessManager::NotAccessible);
  request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,QNetworkRequest::PreferCache);*/
  //load cookies file
  /*QFile f(QMandala::Global::config().filePath(cookiesFileName));
  if (f.open(QIODevice::ReadOnly))
    request.setRawHeader("Cookie",QString(f.readAll()).split("\n",QString::SkipEmptyParts).join(";").toUtf8());*/

  request.setRawHeader("User-Agent",QString("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:%1.0) Gecko/%2%3%4 Firefox/%5.0.%6").arg(QString::number(Random(3,14)), QString::number(Random(QDate().currentDate().year() - 4, QDate().currentDate().year())), QString::number(Random(11,12)), QString::number(Random(10,30)), QString::number(Random(3,14)), QString::number(Random(1,10))).toUtf8());
  request.setRawHeader("Accept","*/*");
  request.setRawHeader("Referrer", "http://maps.google.com/");

  autoDownload=false;
}
//=============================================================================
void MapTiles::download_tile(const QString &tile_name)
{
  const QStringList lp=tile_name.split('_');
  if(lp.size()<3)return;
  /*if(googleMapsVersionSat.isEmpty()){
    //qDebug()<<"req: "<<tile_name;
    if(!req_ver_queue.contains(tile_name))
      req_ver_queue.append(tile_name);
    if(!reqMap.values().contains("ver")){
      //request.setUrl(QUrl("https://maps.google.com/maps?output=classic&amp;dg=brw"));
      request.setUrl(QUrl("https://www.google.com/maps/@?dg=dbrw&newdg=1"));
      reqMap.insert(net.get(request),"ver");
    }
    return;
  }*/
  int x=lp.at(1).toUInt(),y=lp.at(2).toUInt();
  QString sec1 = ""; // after &x=...
  QString sec2 = ""; // after &zoom=...
  getSecGoogleWords(x,y,sec1,sec2);
  QString fname=pathMaps.filePath(tile_name+".jpg");//(force||QImage(fname).isNull()) &&
  if( (!reqMap.values().contains(fname)) && (!downloaded_fnames.contains(fname))){
    QString server = "khms";
    QString req = "kh";
    //QString url=QString("https://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(getServerNum(x,y, 4)).arg(req).arg(googleMapsVersionSat).arg("en").arg(x).arg(sec1).arg(y).arg(level).arg(sec2);
    QString url=QString("https://khm.google.com/vt/lbw/lyrs=y&x=%1&y=%2&z=%3").arg(x).arg(y).arg(level);
    request.setUrl(QUrl(url));
    reqMap.insert(net.get(request),fname);
  }
  //labels
  /*fname=pathMaps.filePath(tile_name+".png");//(force||QImage(fname).isNull()) &&
  if((!reqMap.values().contains(fname)) && (!downloaded_fnames.contains(fname))){
    QString server = "mts";
    QString req = "vt";
    QString url=QString("https://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(getServerNum(x,y, 4)).arg(req).arg(googleMapsVersionLabels).arg("en").arg(x).arg(sec1).arg(y).arg(level).arg(sec2);
    lastUrl=url;
    request.setUrl(QUrl(url));
    reqMap.insert(net.get(request),fname);
  }*/
}
//=============================================================================
void MapTiles::getSecGoogleWords(const int x, const int y,  QString &sec1, QString &sec2) const
{
  const QString sGalileoS="Galileo";
  sec1 = ""; // after &x=...
  sec2 = ""; // after &zoom=...
  int seclen = ((x * 3) + y) % 8;
  sec2 = sGalileoS.left(seclen);
  if(y >= 10000 && y < 100000) sec1 = "&s=";
}
int MapTiles::getServerNum(const int x, const int y, const int max) const
{
    return (x + 2 * y) % max;
}
//=============================================================================
void MapTiles::abort()
{
  downloaded_fnames.clear();
  foreach(QNetworkReply* reply,reqMap.keys()){
    if(reply)reply->abort();
  }
  reqMap.clear();
  qDebug("%s",tr("Download aborted.").toUtf8().data());
  emit downloaded("");
}
//=============================================================================
void MapTiles::replyFinished(QNetworkReply* reply)
{
  QString fname=reqMap.value(reply);
  reqMap.remove(reply);
  if(!reply)return;
  int err=reply->error();
  QByteArray ba=reply->readAll();
  reply->deleteLater();
  //delete reply;
  if(fname.isNull())return;
  //return;

  if(err==QNetworkReply::OperationCanceledError)return;
  if(err) {
    if(err!=203)qWarning("%s (%u).\n",tr("Error downloading map").toUtf8().data(),err);
    //abort();
    return;
  }
  if(fname=="ver"){
    QRegExp reg;
    reg=QRegExp("\"*https://khm\\D?\\d.google.com/kh/v=(\\d*)",Qt::CaseInsensitive);
    if(reg.indexIn(ba)!=-1){
      QStringList gc=reg.capturedTexts();
      googleMapsVersionSat = gc[1];
      qDebug()<<"maps.google.com: "<<googleMapsVersionSat;
    }
    reg=QRegExp("\"*https://mt0.google.com/vt/lyrs=h@(\\d*)",Qt::CaseInsensitive);
    if(reg.indexIn(ba)!=-1){
      QStringList gc=reg.capturedTexts();
      googleMapsVersionLabels = QString("h@%1").arg(gc[1]);
      qDebug()<<"maps.google.com labels: "<<googleMapsVersionLabels;
    }else googleMapsVersionLabels=QString("h@%1").arg(googleMapsVersionSat);
    if(!googleMapsVersionSat.isEmpty()){
      foreach(QString tile_name,req_ver_queue)
        download_tile(tile_name);
      req_ver_queue.clear();
    }
    qDebug()<<"reply2: "<<ba.size();
    QFile f("/home/uavinda/test.htm");
    f.open(QIODevice::WriteOnly);
    f.write(ba);
    f.flush();
    f.close();
    return;
  }

  QImage image;
  image.loadFromData(ba);
  if(image.isNull()||image.size()!=QSize(256,256)){
    QFile::remove(fname);
    qWarning("%s (%s).",tr("Error downloading map").toUtf8().data(),tr("not an image").toUtf8().data());
    abort();
    return;
  }

  QFile f(fname);
  f.open(QIODevice::WriteOnly);
  f.write(ba);
  f.flush();
  f.close();

  downloaded_fnames.append(fname);

  QString tile_name=QFileInfo(fname).baseName();
  /*QStringList st=tile_name.split('_');
  putImageToCache(ba,fname.endsWith(".jpg")?GoogleSatellite:GoogleLabels,QPoint(st.at(1).toUInt(),st.at(2).toUInt()),st.at(0).toUInt());*/
  foreach(MapTile *tile,tiles) {
    if(tile->tile_name!=tile_name)continue;
    tile->reloadImage();
    tile->update();
    break;
  }
  emit downloaded(tile_name);
}
//=============================================================================
void MapTiles::setScene(QGraphicsScene *scene)
{
  this->scene=scene;
}
//=============================================================================
void MapTiles::updateTiles()
{
  if(!(scene && scene->views().size()))return;
  //create missing...
  const int mapscntX=3+view->width()*view->scaleFactor/256;//(view->width()>view->height()?view->width():view->height())/256;
  const int mapscntY=3+view->height()*view->scaleFactor/256;//(view->width()>view->height()?view->width():view->height())/256;
  int dm=tiles.size()-(mapscntX*mapscntY);
  //resized - delete extra tiles
  while(dm>0){
    MapTile *tile=tiles.takeLast();
    disconnect(tile);
    scene->removeItem(tile);
    tile->deleteLater();
    dm--;
  }
  //resized - add more tiles if needed
  while(dm<0){
    MapTile *tile=new MapTile(this);
    if(autoDownload)
      connect(tile,SIGNAL(download_request(QString)),this,SLOT(download_tile(QString)));
    tiles.append(tile);
    scene->addItem(tile);
    dm++;
  }
  //update tiles
  QPoint levelXY=view->mapToScene(view->width()/2,view->height()/2).toPoint();
  int szX=mapscntX/2;
  int szY=mapscntY/2;
  double bsz=NumTiles[maxLevel]/NumTiles[level];
  const int nsz=NumTiles[level];
  int numX=floor(levelXY.x()/bsz);
  int numY=floor(levelXY.y()/bsz);
  QMap<QString,QPoint> pmap;
  for (int y=0;y<mapscntY;y++)
    for (int x=0;x<mapscntX;x++) {
      int nx=numX+x-szX,ny=numY+y-szY;
      if((nx<0)||(ny<0)||(nx>=nsz)||(ny>=nsz))continue;
      QString pkey=QString::number(level)+"_"+QString::number(nx)+"_"+QString::number(ny);
      pmap.insert(pkey,QPoint(nx,ny));
    }
  //reposition available images..
  QList<MapTile*> nlist;
  foreach(MapTile *tile,tiles) {
    QString bname=tile->tile_name;
    if (pmap.contains(bname)){
        tile->upd(pmap.value(bname)*bsz,bsz,bname);
        tile->setVisible(true);
        pmap.remove(bname);
      }else{
        tile->setVisible(false);
        nlist.append(tile);
      }
    }
  //reset other blocks for new images
  foreach(MapTile *tile,nlist) {
    if (!pmap.keys().size())break;
    QString bname=pmap.keys().at(0);
    tile->upd(pmap.value(bname)*bsz,bsz,bname);
    tile->setVisible(true);
    pmap.remove(bname);
  }
}
//=============================================================================
bool MapTiles::updateLevel(double sf)
{
  bool rv=false;
  while(1){
    int sz=NumTiles[maxLevel]/NumTiles[level];
    int bsz=sf*sz;
    if(bsz<256){
      if(level<=minLevel)break;
      level--;
      rv=true;
      continue;
    }
    if(bsz>=512){
      if(level>=maxLevel)break;
      level++;
      rv=true;
      continue;
    }
    break;
  }
  return rv;
}
//=============================================================================
void MapTiles::setAutoDownload(bool v)
{
  autoDownload=v;
  if(v){
    foreach(MapTile *tile,tiles) {
      connect(tile,SIGNAL(download_request(QString)),this,SLOT(download_tile(QString)));
      tile->reloadImage();
    }
  }else foreach(MapTile *tile,tiles) disconnect(tile,SIGNAL(download_request(QString)),this,SLOT(download_tile(QString)));
}
//=============================================================================
void MapTiles::refresh()
{
  foreach(MapTile *tile,tiles) {
    download_tile(tile->tile_name);
  }
}
//=============================================================================
void MapTiles::downloadDeep(QString bname)
{
  QStringList list;
  iterateBlockNames(bname,list);
  foreach(QString tile_name,list)download_tile(tile_name);
}
//=============================================================================
void MapTiles::iterateBlockNames(QString bname,QStringList &list)
{
  QCoreApplication::processEvents();
  list.append(bname);
  const QStringList lp=bname.split('_');
  if(lp.size()!=3)return;
  int bL=lp.at(0).toUInt()+1;
  int bX=lp.at(1).toUInt()*2;
  int bY=lp.at(2).toUInt()*2;
  if(bL>maxLevel)return;
  iterateBlockNames(QString::number(bL)+"_"+QString::number(bX)+"_"+QString::number(bY),list);
  iterateBlockNames(QString::number(bL)+"_"+QString::number(bX+1)+"_"+QString::number(bY),list);
  iterateBlockNames(QString::number(bL)+"_"+QString::number(bX+1)+"_"+QString::number(bY+1),list);
  iterateBlockNames(QString::number(bL)+"_"+QString::number(bX)+"_"+QString::number(bY+1),list);
}
//=============================================================================
/*bool MapTiles::createEmptyDB(const QString &file)
{
  if(QFileInfo(file).exists()) QFile(file).remove();
  QSqlDatabase db;

  db = QSqlDatabase::addDatabase("QSQLITE",QLatin1String("CreateConn"));
  db.setDatabaseName(file);
  if (!db.open()){
    qDebug()<<"CreateEmptyDB: Unable to create database";
    return false;
  }
  QSqlQuery query(db);
  query.exec("CREATE TABLE IF NOT EXISTS Tiles (id INTEGER NOT NULL PRIMARY KEY, X INTEGER NOT NULL, Y INTEGER NOT NULL, Zoom INTEGER NOT NULL, Type INTEGER NOT NULL,Date TEXT)");
  if(query.numRowsAffected()==-1){
    qDebug()<<"CreateEmptyDB: "<<query.lastError().driverText();
    db.close();
    return false;
  }
  query.exec("CREATE TABLE IF NOT EXISTS TilesData (id INTEGER NOT NULL PRIMARY KEY CONSTRAINT fk_Tiles_id REFERENCES Tiles(id) ON DELETE CASCADE, Tile BLOB NULL)");
  if(query.numRowsAffected()==-1){
    qDebug()<<"CreateEmptyDB: "<<query.lastError().driverText();
    db.close();
    return false;
  }
  query.exec(
          "CREATE TRIGGER fki_TilesData_id_Tiles_id "
          "BEFORE INSERT ON [TilesData] "
          "FOR EACH ROW BEGIN "
          "SELECT RAISE(ROLLBACK, 'insert on table TilesData violates foreign key constraint fki_TilesData_id_Tiles_id') "
          "WHERE (SELECT id FROM Tiles WHERE id = NEW.id) IS NULL; "
          "END");
  if(query.numRowsAffected()==-1){
    qDebug()<<"CreateEmptyDB: "<<query.lastError().driverText();
    db.close();
    return false;
  }
  query.exec(
          "CREATE TRIGGER fku_TilesData_id_Tiles_id "
          "BEFORE UPDATE ON [TilesData] "
          "FOR EACH ROW BEGIN "
          "SELECT RAISE(ROLLBACK, 'update on table TilesData violates foreign key constraint fku_TilesData_id_Tiles_id') "
          "WHERE (SELECT id FROM Tiles WHERE id = NEW.id) IS NULL; "
          "END");
  if(query.numRowsAffected()==-1){
    qDebug()<<"CreateEmptyDB: "<<query.lastError().driverText();
    db.close();
    return false;
  }
  query.exec(
          "CREATE TRIGGER fkdc_TilesData_id_Tiles_id "
          "BEFORE DELETE ON Tiles "
          "FOR EACH ROW BEGIN "
          "DELETE FROM TilesData WHERE TilesData.id = OLD.id; "
          "END");
  if(query.numRowsAffected()==-1){
    qDebug()<<"CreateEmptyDB: "<<query.lastError().driverText();
    db.close();
    return false;
  }
  db.close();
  QSqlDatabase::removeDatabase(QLatin1String("CreateConn"));
  return true;
}
//=============================================================================
bool MapTiles::putImageToCache(const QByteArray &tile, const _maptype &type, const QPoint &pos, const int &zoom)
{
  if(dbfile.isEmpty())return false;
  lock.lockForRead();
  //qDebug()<<"PutImageToCache Start:";//<<pos;
  Mcounter.lock();
  qlonglong id=++ConnCounter;
  Mcounter.unlock();
  {
    QSqlDatabase cn = QSqlDatabase::addDatabase("QSQLITE",QString::number(id));
    cn.setDatabaseName(dbfile);
    cn.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    if(cn.open()){
      {
        QSqlQuery query(cn);
        query.prepare("INSERT INTO Tiles(X, Y, Zoom, Type,Date) VALUES(?, ?, ?, ?,?)");
        query.addBindValue(pos.x());
        query.addBindValue(pos.y());
        query.addBindValue(zoom);

        query.addBindValue((int)type);
        query.addBindValue(QDateTime::currentDateTime().toString());
        query.exec();
      }
      {
        QSqlQuery query(cn);
        query.prepare("INSERT INTO TilesData(id, Tile) VALUES((SELECT last_insert_rowid()), ?)");
        query.addBindValue(tile);
        query.exec();
      }
      cn.close();
    }
  }
  QSqlDatabase::removeDatabase(QString::number(id));
  lock.unlock();
  return true;
}
//=============================================================================
QByteArray MapTiles::getImageFromCache(_maptype type, QPoint pos, int zoom)
{
  lock.lockForRead();
  QByteArray ar;
  if(dbfile.isEmpty()) return ar;
  Mcounter.lock();
  qlonglong id=++ConnCounter;
  Mcounter.unlock();
  //qDebug()<<"Cache dir="<<dir<<" Try to GET:"<<pos.X()+","+pos.Y();
  {
    QSqlDatabase cn = QSqlDatabase::addDatabase("QSQLITE",QString::number(id));
    cn.setDatabaseName(dbfile);
    cn.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    if(cn.open()){
      QSqlQuery query(cn);
      query.exec(QString("SELECT Tile FROM TilesData WHERE id = (SELECT id FROM Tiles WHERE X=%1 AND Y=%2 AND Zoom=%3 AND Type=%4)").arg(pos.x()).arg(pos.y()).arg(zoom).arg((int) type));
      query.next();
      if(query.isValid()) ar=query.value(0).toByteArray();
      cn.close();
    }
  }
  QSqlDatabase::removeDatabase(QString::number(id));
  lock.unlock();
  return ar;
}
//=============================================================================
void MapTiles::deleteOlderTiles(int const& days)
{
  if(!QFileInfo(dbfile).exists())return;
  QSqlDatabase cn;
  Mcounter.lock();
  qlonglong id=++ConnCounter;
  Mcounter.unlock();
  cn = QSqlDatabase::addDatabase("QSQLITE",QString::number(id));
  cn.setDatabaseName(dbfile);
  cn.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
  if(cn.open()){
    QList<long> add;
    QSqlQuery query(cn);
    query.exec(QString("SELECT id, X, Y, Zoom, Type, Date FROM Tiles"));
    while(query.next()){
      if(QDateTime::fromString(query.value(5).toString()).daysTo(QDateTime::currentDateTime())>days)
          add.append(query.value(0).toLongLong());
    }
    foreach(long i,add) query.exec(QString("DELETE FROM Tiles WHERE id = %1;").arg(i));
    cn.close();
  }
  QSqlDatabase::removeDatabase(QString::number(id));
}*/
//=============================================================================
