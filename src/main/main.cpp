﻿/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <QApplication>
#include <qdesktopwidget.h>
#include <QtCore>
#include <QGLWidget>
#include <QQuickStyle>
#include <QtQuick>
#include <SvgMaterialIcon.h>
#include "plugin_interface.h"
#include "QMandala.h"
#include "MainForm.h"
#include "Serial.h"
#include "HttpService.h"
#include <QTranslator>
#include "MsgTranslator.h"
#include "RunGuard.h"
#include "SoundEffects.h"
#include "AppShortcuts.h"
#include "FactSystem.h"
#include "AppSettings.h"
#include "Datalink.h"
#include "AppDirs.h"
#include "Vehicles.h"
#include "DatabaseConnection.h"
//============================================================================
//global variables
QMandala *mandala;
//DatalinkServer *datalink;
//MandalaTree *m;
//----------------------------------------------------------------------------
void loadShortcuts();
void loadPlugins();
void loadFonts();
void checkPaths();
//----------------------------------------------------------------------------
MainForm *mainForm;
//Joystick *joystick;
//Serial *serial1=NULL;
//Serial *serial2=NULL;
HttpService *httpService;
//============================================================================
/*void crash_handler(int sig) {
 fprintf(stdout,"\nCRASH\n");
 if(serial1)delete serial1;
 if(serial2)delete serial2;
 exit(-1);
}*/
//============================================================================
int main(int argc, char *argv[])
{
#ifdef Q_OS_MAC
  //qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "false");
  qputenv("QT_SCALE_FACTOR", "1");
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling,false);
  //QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps,false);
  //QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
  //QApplication::setAttribute(Qt::AA_Use96Dpi);
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

#endif

  //signal(SIGSEGV, crash_handler); //installing signal handler
  //signal(SIGTERM, crash_handler); //installing signal handler

  //qputenv("QT_QUICK_CONTROLS_STYLE", "Android");

  //qputenv("LD_LIBRARY_PATH", "/usr/lib/uavos/Qt/lib/");
  //QString qmlcache = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
  //QDir qmlCacheDir(qmlcache);
  //qmlCacheDir.removeRecursively();

  //QApplication::setGraphicsSystem(QLatin1String("opengl"));
  //QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName("uavos");
  QCoreApplication::setOrganizationDomain("uavos.com");
  QCoreApplication::setApplicationName("gcs");
  QSettings().setDefaultFormat(QSettings::IniFormat);

  FactSystem *factSystem=new FactSystem();

  new AppSettings(factSystem);
  Datalink *datalink=new Datalink(factSystem);

  Vehicles *vehicles=new Vehicles(factSystem);

  QObject::connect(datalink,&Datalink::read,vehicles,&Vehicles::downlinkReceived);
  QObject::connect(vehicles,&Vehicles::sendUplink,datalink,&Datalink::write);



  //load static plugin
  Q_IMPORT_PLUGIN(GeoPlugin)




  if(QSettings().value("qsg_basic").toBool()){
    qputenv("QSG_RENDER_LOOP","basic");
  }
  //qputenv("QSG_RENDER_LOOP","threaded");
  //qputenv("QSG_INFO","1");

  QQuickStyle::setStyle("Material");

  //translate lang
  QString lang=AppSettings::value("lang").toString();
  if(QLocale().country()==QLocale::Belarus&&(lang=="ru")){
    lang="by";
    AppSettings::setValue("lang",lang);
  }
  QDir langp(AppDirs::lang());
  QString langf;
  langf=langp.filePath(lang+".qm");
  if(QFile::exists(langf)){
    QTranslator *translator=new QTranslator();
    translator->load(langf);
    app.installTranslator(translator);
    qDebug("Translator added: %s",langf.toUtf8().data());
  }
  QDir langsp("/usr/share/qt5/translations/");
  QString qt_langf=langsp.filePath("qt_"+lang+".qm");
  if(QFile::exists(qt_langf)){
    QTranslator *translator=new QTranslator();
    translator->load(qt_langf);
    app.installTranslator(translator);
    qDebug("Translator added: %s",qt_langf.toUtf8().data());
  }


  QGLFormat f = QGLFormat::defaultFormat();
  f.setSwapInterval(0);
  f.setSampleBuffers(true);
  f.setSamples(2);
  QGLFormat::setDefaultFormat(f);

  //Settings
  //Config::defaults(false);

  //check instances
  if(!QSettings().value("multipleInstances").toBool()){
    RunGuard guard("instance.gcs.uavos.com");
    if(!guard.tryToRun()){
      qWarning("%s",QObject::tr("Another application instance is running").toUtf8().data());
      if(!QSettings().value("multipleInstances").toBool()){
        return 0;
      }
    }
  }

  loadFonts();

  SvgMaterialIcon::initialize();
  //qmlRegisterSingletonType<SvgMaterialIcon>("GCS.SvgMaterialIcon", 1, 0, "SvgMaterialIcon", fooThatReturnsValidatorPtr);

//#ifdef Q_OS_MAC
  //qApp->setFont(QFont("Avenir",10));
//#endif


  // Load the new stylesheet.
  QFile styleSheet(":styles/style-old.css");
  if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)){
    qApp->setStyleSheet(styleSheet.readAll());
    //qDebug()<<styleSheet.fileName();
  }

  //qDebug()<<QStyleFactory::keys();
#ifdef Q_OS_MAC
  QStyle *style=QStyleFactory::create("Fusion");
#else
  QStyle *style=QStyleFactory::create("Breeze");
#endif
  if(style)qApp->setStyle(style);

  //create main objects
  mandala=new QMandala;

  //factSystem->loadMandala(mandala->current);
  factSystem->jsSyncObject(mandala);


  //Http service
  httpService=new HttpService();
  QObject::connect(datalink,&Datalink::httpRequest,httpService,&HttpService::httpRequest);

  // directories..
  checkPaths();

  if(QCoreApplication::arguments().contains("-x"))
    QSettings().setValue("maximized", true);

  // main window..
  mainForm=new MainForm();
  mainForm->setWindowTitle(QObject::tr("Ground Control Unit"));
  if(FactSystem::devMode())
    mainForm->setWindowTitle(mainForm->windowTitle()+" ("+FactSystem::version()+")");

  //hotkeys
  new AppShortcuts(AppSettings::instance(),mainForm);



  //PLUGINS
  loadPlugins();

  //other objects
  //joystick=new Joystick();

  SoundEffects soundEffects;
  QObject::connect(factSystem,&FactSystem::playSoundEffect,&soundEffects,&SoundEffects::play);


  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  exec
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  int rv=app.exec();
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  qInstallMessageHandler(0);

  /*foreach (QString s, QSqlDatabase::connectionNames()) {
    qDebug()<<s;
    QSqlDatabase::database(s,false).close();
    QSqlDatabase::removeDatabase(s);
  }*/

  foreach (DatabaseConnection *db, DatabaseConnection::connections) {
    delete db;
  }

  //delete factSystem;

  return rv;
}
/*catch (std::exception & e)
{
  fprintf(stdout,"\nCATCH1\n");
  delete serial1;
  delete serial2;
}
catch (...)
{
  fprintf(stdout,"\nCATCH2\n");
  // someone threw something undecypherable
  delete serial1;
  delete serial2;
}*/
//============================================================================
void loadFonts()
{
  QFile res;
  res.setFileName(":/fonts/BebasNeue.otf");
  if(res.open(QIODevice::ReadOnly)){
    QFontDatabase::addApplicationFontFromData(res.readAll());
    res.close();
  }
  res.setFileName(":/fonts/FreeMono.ttf");
  if(res.open(QIODevice::ReadOnly)){
    QFontDatabase::addApplicationFontFromData(res.readAll());
    res.close();
  }
  res.setFileName(":/fonts/FreeMonoBold.ttf");
  if(res.open(QIODevice::ReadOnly)){
    QFontDatabase::addApplicationFontFromData(res.readAll());
    res.close();
  }
  res.setFileName(":/fonts/Ubuntu-C.ttf");
  if(res.open(QIODevice::ReadOnly)){
    QFontDatabase::addApplicationFontFromData(res.readAll());
    res.close();
  }
  res.setFileName(":/fonts/Bierahinia.ttf");
  if(res.open(QIODevice::ReadOnly)){
    QFontDatabase::addApplicationFontFromData(res.readAll());
    res.close();
  }
}
//=============================================================================
void loadPlugins()
{
  //collect all available plugins filenames
  QStringList allFiles;
  QStringList filters(QStringList()<<"*.so"<<"*.dylib"<<"*.qml");

  QDir userp=AppDirs::userPlugins();
  if(!userp.exists())userp.mkpath(".");
  QStringList stRep,stRepQml;
  foreach (QString fileName, userp.entryList(filters,QDir::Files)){
    if(fileName.startsWith('-'))continue;
    QString pname=QString(fileName).remove("lib");
    if(!pname.endsWith(".qml")){
      pname.truncate(pname.lastIndexOf('.'));
      stRep.append(pname);
    }else{
      stRepQml.append(pname.left(pname.lastIndexOf('.')));
    }
    allFiles.append(userp.absoluteFilePath(fileName));
  }
  if(!stRep.isEmpty())qDebug("%s: %s",QObject::tr("User plugins").toUtf8().data(),stRep.join(',').toUtf8().data());
  if(!stRepQml.isEmpty())qDebug("%s: %s",QObject::tr("User QML plugins").toUtf8().data(),stRepQml.join(',').toUtf8().data());

  QDir pluginsDir=AppDirs::plugins();
  //qDebug()<<pluginsDir;
  foreach (QString fileName,pluginsDir.entryList(filters,QDir::Files)){
    //qDebug()<<fileName;
    allFiles.append(pluginsDir.absoluteFilePath(fileName));
  }

  allFiles.append("qrc:///video.qml");
  (void)QT_TRANSLATE_NOOP("Plugins","Video");

  allFiles.append("qrc:///EFIS.qml");
  (void)QT_TRANSLATE_NOOP("Plugins","EFIS");

  allFiles.append("qrc:///HDG.qml");
  (void)QT_TRANSLATE_NOOP("Plugins","HDG");

  allFiles.append("qrc:///Facts.qml");
  (void)QT_TRANSLATE_NOOP("Plugins","Controls");

  allFiles.append("qrc:///Rotax914.qml");

  allFiles.append("qrc:///Map.qml");

  allFiles.append("qrc:///Signals.qml");

  //allFiles.append("qrc:///videoCV.qml");


  //parse command line arguments (plugins to load -> cmd_plist)
  QStringList cmd_plist;
  for(int i=0;i<QCoreApplication::arguments().size();i++){
    QString s=QCoreApplication::arguments().at(i).trimmed();
    if(!s.size())continue;
    if(s.at(0)!='-')continue;
    if(s=="-p"){
      for(++i;i<QCoreApplication::arguments().size();i++){
        QString s=QCoreApplication::arguments().at(i).trimmed();
        if(!s.size())continue;
        if(s.at(0)=='-'){
          i--;
          break;
        }
        cmd_plist.append(s);
      }
      continue;
    }
  }

  //parse settings - create filtered plugins list
  QSettings st;
  st.beginGroup("plugins");
  QStringList pfiles;
  foreach (QString fileName, allFiles) {
    QString pname=QFileInfo(fileName).baseName().remove("lib");
    if(cmd_plist.size()&&(!cmd_plist.contains(pname)))continue;
    if(st.value(pname).toString()=="off"){
      qDebug("%s: %s",QObject::tr("Plugin excluded").toUtf8().data(),pname.toUtf8().data());
      continue;
    }else if(st.value(pname).toString()=="on"){
      st.setValue(pname,"true");
    }
    pfiles.append(fileName);
  }

  qDebug("%s...",QObject::tr("Loading plugins").toUtf8().data());
  mainForm->setPlugins(pfiles);
}
//============================================================================
//============================================================================
void linkFiles(QDir src,QDir dest)
{
  if(!dest.exists()) dest.mkpath(".");
  foreach(QFileInfo fi,src.entryInfoList(QDir::Files)){
    QFileInfo fiDest(dest.filePath(fi.fileName()));
    if(fiDest.exists() || fiDest.isSymLink()){
      if(fiDest.isSymLink() && fiDest.symLinkTarget()==fi.absoluteFilePath())continue;
      QFile::remove(fiDest.absoluteFilePath());
    }
    QFile::link(fi.absoluteFilePath(),fiDest.absoluteFilePath());
    qDebug("Link updated: %s",fiDest.absoluteFilePath().toUtf8().data());
  }
}
void linkDir(QDir src,QDir dest,QString suffix)
{
  if(!dest.exists()) dest.mkpath(".");
  QFileInfo fiDest(dest.filePath(src.dirName()+suffix));
  if(fiDest.exists() || fiDest.isSymLink()){
    if(fiDest.isSymLink() && fiDest.symLinkTarget()==src.absolutePath())return;
    QFile::remove(fiDest.absoluteFilePath());
  }
  QFile::link(src.absolutePath(),fiDest.absoluteFilePath());
  qDebug("Link updated: %s",fiDest.absoluteFilePath().toUtf8().data());
}
bool copyPath(QDir src,QDir dest,bool deleteOnCopy=false)
{
  if(!src.exists()) return false;
  if(!dest.exists()) dest.mkpath(".");

  //qDebug()<<"copyPath"<<src.path()<<dest.path();
  foreach(QFileInfo fi,src.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot)){
    dest.mkpath(fi.fileName());
    if(!src.cd(fi.fileName()))return false;
    if(!dest.cd(fi.fileName()))return false;
    if(!copyPath(src,dest,deleteOnCopy))return false; //recursion
    src.cdUp();
    dest.cdUp();
  }
  //qDebug()<<"files"<<src.absolutePath();
  uint fcnt=0;
  foreach(QFileInfo fi,src.entryInfoList(QDir::Files)){
    //qDebug()<<"file"<<fi.absoluteFilePath();
    const QString fndest(dest.absoluteFilePath(fi.fileName()));
    if(QFile::exists(fndest))continue;
    //QFile::copy(fi.absoluteFilePath(),fndest);
    //if(deleteOnCopy)QFile::remove(fi.absoluteFilePath());
    if(deleteOnCopy)QFile::rename(fi.absoluteFilePath(),dest.absoluteFilePath(fi.fileName()));
    else QFile::copy(fi.absoluteFilePath(),fndest);
    fcnt++;
  }
  // Possible race-condition mitigation?
  dest.refresh();
  if(!dest.exists()) return false;
  if(fcnt>0){
    qDebug("Updated %u user files in %s",fcnt,dest.absolutePath().toUtf8().data());
  }
  return true;
}
bool movePath(QDir src,QDir dest)
{
  if((!dest.exists())||dest.isEmpty()){
    dest.mkpath(".");
    QDir d(dest);
    d.cdUp();
    d.rmdir(dest.dirName());
    if(QDir().rename(src.absolutePath(),dest.absolutePath())){
      qWarning("Moved: '%s' to '%s'",src.path().toUtf8().data(),dest.path().toUtf8().data());
      return true;
    }
  }
  if(copyPath(src,dest,true)){
    if(src.rmpath(".")) return true;
  }
  return false;
}
void fixDeprecatedPath(QString srcPath,QDir dest)
{
  QDir src(QDir(QDir::home().absoluteFilePath(".gcu")).absoluteFilePath(srcPath));
  if(!src.exists())return;
  if(movePath(src,dest))return;
  qDebug("Deprecated user path: %s",src.absolutePath().toUtf8().data());
}
void checkPaths()
{
  //qDebug()<<"checkPaths";

  if(!AppDirs::db().exists())AppDirs::db().mkpath(".");

  //fix old paths
  fixDeprecatedPath("config/uav.conf.d",AppDirs::configs());
  fixDeprecatedPath("flightplans",AppDirs::missions());
  fixDeprecatedPath("maps",QDir(AppDirs::maps().absoluteFilePath("google-tiles")));
  fixDeprecatedPath("nodes",AppDirs::nodes());
  fixDeprecatedPath("plugins",AppDirs::userPlugins());
  fixDeprecatedPath("scripts",AppDirs::scripts());
  fixDeprecatedPath("data",AppDirs::telemetry());

  // link sample files
  linkFiles(AppDirs::res().absoluteFilePath("nodes/sample-configs"),AppDirs::configs());
  linkFiles(AppDirs::res().absoluteFilePath("missions"),AppDirs::missions());
  linkFiles(AppDirs::res().absoluteFilePath("telemetry"),AppDirs::telemetry());
  linkDir(AppDirs::res().absoluteFilePath("scripts/pawn"),AppDirs::scripts(),"-examples");

  //warn if exists old dir
  QDir src(QDir::home().absoluteFilePath(".gcu"));
  if(!src.exists())return;
  if(src.isEmpty() && src.rmdir("."))return;
  qWarning("Deprecated storage directory: %s",src.absolutePath().toUtf8().data());
  qWarning("New storage directory: %s",AppDirs::user().absolutePath().toUtf8().data());
}
//============================================================================

