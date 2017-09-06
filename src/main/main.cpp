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
#include "plugin_interface.h"
#include "QMandala.h"
#include "MainForm.h"
#include "Serial.h"
#include "DatalinkServer.h"
#include "Config.h"
#include "HttpService.h"
#include <QTranslator>
#include "MsgTranslator.h"
#include "RunGuard.h"
#include "SoundEffects.h"
//============================================================================
//global variables
QMandala *mandala;
//DatalinkServer *datalink;
//MandalaTree *m;
//----------------------------------------------------------------------------
void loadShortcuts();
void loadPlugins();
void loadFonts();
void rsyncLib();
//----------------------------------------------------------------------------
MainForm *mainForm;
//Joystick *joystick;
Serial *serial1=NULL;
Serial *serial2=NULL;
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
  QString qmlcache = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
  QDir qmlCacheDir(qmlcache);
  qmlCacheDir.removeRecursively();
  qmlRegisterType<DatalinkServer>();



  //Q_INIT_RESOURCE(gcu);
  //QApplication::setGraphicsSystem(QLatin1String("opengl"));
  QApplication app(argc, argv);

#ifdef Q_OS_MAC
  qApp->setFont(QFont("Ubuntu",10));
#endif

  QCoreApplication::setOrganizationName("uavos.com");
  QCoreApplication::setOrganizationDomain("www.uavos.com");
  QCoreApplication::setApplicationName("GCU");
  QSettings().setDefaultFormat(QSettings::IniFormat);

  if(QSettings().value("qsg_basic").toBool()){
    qputenv("QSG_RENDER_LOOP","basic");
  }
  //qputenv("QSG_RENDER_LOOP","threaded");
  //qputenv("QSG_INFO","1");

  QQuickStyle::setStyle("Material");

  //translate lang
  if(QLocale().country()==QLocale::Belarus&&(QSettings().value("lang").toString()=="ru"))
    QSettings().setValue("lang","by");
  QDir langp(QMandala::Global::lang());
  QString langf;
  /*langf=langp.filePath(QSettings().value("lang").toString()+"_msg.ts");
  if(QFile::exists(langf)){
    MsgTranslator *translator=new MsgTranslator();
    translator->loadXml(langf);
    app.installTranslator(translator);
    qDebug("Translator added: %s",langf.toUtf8().data());
  }*/
  langf=langp.filePath(QSettings().value("lang").toString()+".qm");
  if(QFile::exists(langf)){
    QTranslator *translator=new QTranslator();
    translator->load(langf);
    app.installTranslator(translator);
    qDebug("Translator added: %s",langf.toUtf8().data());
  }
  QDir langsp("/usr/share/qt5/translations/");
  QString qt_langf=langsp.filePath("qt_"+QSettings().value("lang").toString()+".qm");
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
  Config::defaults(false);

  //check instances
  if(!QSettings().value("multipleInstances").toBool()){
    RunGuard guard("instance.gcu.uavos.com");
    if(!guard.tryToRun()){
      qWarning("%s",QObject::tr("Another application instance is running").toUtf8().data());
      if(!QSettings().value("multipleInstances").toBool()){
        return 0;
      }
    }
  }

  loadFonts();

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

  DatalinkServer *datalink=new DatalinkServer;

  QObject::connect(datalink,SIGNAL(dataReceived(QByteArray)),mandala,SLOT(downlinkReceived(QByteArray)));
  QObject::connect(mandala,SIGNAL(sendUplink(QByteArray)),datalink,SLOT(dataSend(QByteArray)));

  //QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),var->rec,SLOT(record_uplink(QByteArray)));

  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),mandala,SLOT(upCntInc()));
  QObject::connect(datalink,SIGNAL(dataReceived(QByteArray)),mandala,SLOT(dlCntInc()));

  QObject::connect(mandala,SIGNAL(readOnlyChanged(bool)),datalink,SLOT(setReadOnly(bool)));

  //gate serial ports?
  //QObject::connect(serial1,SIGNAL(received(QByteArray)),serial2,SLOT(send(QByteArray)));

  //wrapper/forwarder
  QObject::connect(datalink,SIGNAL(serverDiscovered(QHostAddress,QString)),mandala,SIGNAL(serverDiscovered(QHostAddress,QString)));
  QObject::connect(mandala,SIGNAL(connectToServer(QHostAddress)),datalink,SLOT(connectToServer(QHostAddress)));

  //Http service
  httpService=new HttpService();
  QObject::connect(datalink,SIGNAL(httpRequest(QTextStream&,QString,bool*)),httpService,SLOT(httpRequest(QTextStream&,QString,bool*)));

  // directories..
  if(QMandala::Global::devMode()) qDebug("%s",QObject::tr("Developer mode").toUtf8().data());
  else
    rsyncLib();

  if(QCoreApplication::arguments().contains("-x"))
    QSettings().setValue("maximized", true);

  // main window..
  mainForm=new MainForm();
  mainForm->setWindowTitle(QObject::tr("Ground Control Unit"));
  if(QMandala::Global::devMode())
    mainForm->setWindowTitle(mainForm->windowTitle()+" ("+QMandala::version+")");
  qDebug("%s: %s",QObject::tr("Version").toUtf8().data(),QMandala::version.toUtf8().data());

  //QObject::connect(mainForm,SIGNAL(pluginsLoaded()),datalink,SLOT(activate()));

  //PLUGINS
  loadPlugins();

  //other objects
  //joystick=new Joystick();

  serial1=new Serial(0,qApp,false);
  QObject::connect(serial1,SIGNAL(received(QByteArray)),datalink,SLOT(localDataReceived(QByteArray)));
  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),serial1,SLOT(send(QByteArray)));
  QObject::connect(datalink,SIGNAL(heartbeat(QByteArray)),serial1,SLOT(send(QByteArray)));
  //QObject::connect(mainForm,SIGNAL(pluginsLoaded()),serial1,SLOT(activate()));

  serial2=new Serial(1,qApp,false);
  QObject::connect(serial2,SIGNAL(received(QByteArray)),datalink,SLOT(localDataReceived(QByteArray)));
  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),serial2,SLOT(send(QByteArray)));
  QObject::connect(datalink,SIGNAL(heartbeat(QByteArray)),serial2,SLOT(send(QByteArray)));
  //QObject::connect(mainForm,SIGNAL(pluginsLoaded()),serial2,SLOT(activate()));


  SoundEffects *soundEffects=new SoundEffects(mandala);
  QObject::connect(mandala,SIGNAL(playSoundEffect(QString)),soundEffects,SLOT(play(QString)));

  datalink->activate();
  serial1->activate();
  serial2->activate();

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  exec
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  int rv=app.exec();
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  qInstallMessageHandler(0);

  //serial1->close();
  //serial2->close();

  delete serial1;
  delete serial2;

  /*delete httpService;
  delete mainForm;
  delete datalink;
  delete serial1;
  delete serial2;
  delete joystick;
  delete mandala;*/
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
  QFile res(":/fonts/BebasNeue.otf");
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
  QDir userp=QMandala::Global::local();
  if(userp.cd("plugins")){
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
  }
  QDir pluginsDir=QMandala::Global::plugins();
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

  allFiles.append("qrc:///controls.qml");
  (void)QT_TRANSLATE_NOOP("Plugins","Controls");

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
void rsyncLib()
{
  // update user config files if needed..
  if(!QMandala::Global::config().exists()) QMandala::Global::config().mkpath(".");
  QDir cfg_src(QMandala::Global::res().absoluteFilePath("config"));
  QFileInfoList list=cfg_src.entryInfoList(QStringList()<<"*",QDir::Files);
  foreach(QFileInfo fi,list){
    QFileInfo fiLocal(QMandala::Global::config().filePath(fi.fileName()));
    if(!fiLocal.exists() || fiLocal.lastModified()<fi.lastModified()){
      QFile::remove(fiLocal.absoluteFilePath());
      QFile::copy(fi.absoluteFilePath(),fiLocal.absoluteFilePath());
      qDebug("config file updated: %s",fiLocal.fileName().toUtf8().data());
    }
  }

  // update uav.conf.d
  if(!QMandala::Global::uav().exists()) QMandala::Global::uav().mkpath(".");

  //check xplane-sim.uav links
  QDir pfLib(QMandala::Global::res());
  QDir pfLocal(QMandala::Global::uav());
  if(pfLib.cd(QMandala::Global::config().dirName()) && pfLib.cd(pfLocal.dirName())){
    foreach(QFileInfo fi,pfLib.entryInfoList(QStringList()<<"*.nodes",QDir::Files)){
      QFileInfo fiLocal(pfLocal.filePath(fi.fileName()));
      if(fiLocal.exists() && fiLocal.isSymLink())continue;
      if(!QFile::rename(fiLocal.absoluteFilePath(),fiLocal.absoluteDir().filePath(fiLocal.baseName()+".bak.nodes")))
        QFile::remove(fiLocal.absoluteFilePath());
      QFile::link(fi.absoluteFilePath(),fiLocal.absoluteFilePath());
      qDebug("uav config file updated: %s",fiLocal.fileName().toUtf8().data());
    }
  }

  //default flightplans
  QDir pwLib(QMandala::Global::res());
  QDir pwLocal(QMandala::Global::missions());
  if(!pwLocal.exists())pwLocal.mkpath(".");
  if(pwLib.cd(pwLocal.dirName())){
    foreach(QFileInfo fi,pwLib.entryInfoList(QDir::Files)){
      QFileInfo fiLocal(pwLocal.filePath(fi.fileName()));
      if(fiLocal.exists() && fiLocal.isSymLink())continue;
      QFile::link(fi.absoluteFilePath(),fiLocal.absoluteFilePath());
      qDebug("flight plan file updated: %s",fiLocal.fileName().toUtf8().data());
    }
  }

  // update scr files if needed..
  if(!QMandala::Global::scr().exists()) QMandala::Global::scr().mkpath(".");
  QDir scr_src(QMandala::Global::res().absoluteFilePath("scripts"));
  QFileInfoList scr_list=scr_src.entryInfoList(QStringList()<<"*",QDir::Files);
  foreach(QFileInfo fi,scr_list){
    QFileInfo fiLocal(QMandala::Global::scr().filePath(fi.fileName()));
    if(!fiLocal.exists()){
      QFile::link(fi.absoluteFilePath(),fiLocal.absoluteFilePath());
      qDebug("script file updated: %s",fiLocal.fileName().toUtf8().data());
    }
  }


  //sample telemetry data
  QDir ptLib(QMandala::Global::res());
  QDir ptLocal(QMandala::Global::records());
  if(!ptLocal.exists())ptLocal.mkpath(".");
  if(ptLib.cd(ptLocal.dirName())){
    foreach(QFileInfo fi,ptLib.entryInfoList(QDir::Files)){
      QFileInfo fiLocal(ptLocal.filePath(fi.fileName()));
      if(fiLocal.exists())continue;
      QFile::link(fi.absoluteFilePath(),fiLocal.absoluteFilePath());
      qDebug("sample telemetry file updated: %s",fiLocal.fileName().toUtf8().data());
    }
  }
}
//============================================================================

