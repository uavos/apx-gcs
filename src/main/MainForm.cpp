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
#include <QtQuick>
#include <QQuickView>
#include <QGLWidget>
#include "MainForm.h"
#include "QMandala.h"
#include "Config.h"
#include "QmlView.h"
#include "DatalinkServer.h"
#include "AppSettings.h"
//=============================================================================
MainForm::MainForm(QWidget *parent)
  : QMainWindow(parent),
  closing(false),loading(true)
{
  mandala=qApp->property("Mandala").value<QMandala*>();
  datalink=qApp->property("DatalinkServer").value<DatalinkServer*>();
  setDockNestingEnabled(true);
  //setDockOptions(dockOptions()&(~QMainWindow::AnimatedDocks)); <- BUG in yakkety?
  //setTabPosition(Qt::AllDockWidgetAreas,QTabWidget::North);

  mFile=menuBar()->addMenu(tr("&File"));
  mUAV=menuBar()->addMenu(tr("&UAV"));
  mUAV->setEnabled(false);
  //mEdit=menuBar()->addMenu(tr("T&elemetry"));
  mTools=menuBar()->addMenu(tr("&Tools"));
  mWindow=menuBar()->addMenu(tr("&Window"));
  mHelp=menuBar()->addMenu(tr("&Help"));

  connect(mandala,SIGNAL(serverDiscovered(QHostAddress,QString)),this,SLOT(serverDiscovered(QHostAddress,QString)));
  connect(this,SIGNAL(connectToServer(QHostAddress)),mandala,SIGNAL(connectToServer(QHostAddress)));

  connect(mandala,SIGNAL(uavAdded(QMandalaItem*)),this,SLOT(uavAdded(QMandalaItem*)));
  connect(mandala,SIGNAL(uavRemoved(QMandalaItem*)),this,SLOT(uavRemoved(QMandalaItem*)));
  connect(this,SIGNAL(changeUAV(QMandalaItem*)),mandala,SLOT(setCurrent(QMandalaItem*)));

  mTools->addAction(QIcon(":/icons/old/configure.png"),tr("System Configuration"),this,SLOT(mSystem_triggered()));
  mTools->addSeparator();

  mWindow->addAction(QIcon(":/icons/old/view-fullscreen.png"),tr("Toggle Full Screen"),this,SLOT(mFullScreen_triggered()),QKeySequence("F11"));
  mWindow->addAction(tr("Auto arrange"),this,SLOT(arrange()));
  //mWindow->addAction(tr("Unlock widgets"),this,SLOT(unlock()));
  //mWindow->addAction(tr("Lock widgets"),this,SLOT(lock()));
  mWindow->addSeparator();

  mHelp->addAction(QIcon(":/icons/old/application-pdf.png"),tr("Mandala Report"),this,SLOT(mMandala_triggered()));
  mHelp->addAction(QIcon(":/icons/old/text-html.png"),tr("Documentation"),this,SLOT(mDoc_triggered()));
  mHelp->addSeparator();
  mHelp->addAction(QIcon(":/icons/old/connect_creating.png"),tr("VPN support"),this,SLOT(mVPN_triggered()));
  connect(&vpnProcess,SIGNAL(finished(int,QProcess::ExitStatus)),SLOT(vpn_disconnected()));
  vpnProcess.setEnvironment(QProcess::systemEnvironment());

  mHelp->addSeparator();
  mHelp->addAction(QString(QMandala::version));

  //app menu actions
  QAction *a;
  a=new QAction(tr("Record data"),this);
  a->setCheckable(true);
  a->setChecked(mandala->current->rec->recording());
  connect(a,SIGNAL(triggered(bool)),mandala->current->rec,SLOT(setRecording(bool)));
  connect(mandala->current->rec,SIGNAL(recordingChanged(bool)),a,SLOT(setChecked(bool)));
  mFile->addAction(a);
  //mFile->addAction(QIcon(":/icons/old/transport_range.png"),tr("Record new file"),var->rec,SLOT(close()));
  mFile->addAction(QIcon(":/icons/old/transport_loop.png"),tr("Discard current file"),mandala->current->rec,SLOT(discard()));
  mFile->addSeparator();

  /*a=new QAction(tr("Sounds"),this);
  a->setCheckable(true);
  a->setChecked(AppSettings::value("sounds").toBool());
  connect(a,SIGNAL(triggered(bool)),AppSettings::fact("sounds"),SLOT(setValue(bool)));
  connect(AppSettings::fact("sounds"),SIGNAL(valueChanged(bool)),a,SLOT(setChecked(bool)));
  mFile->addAction(a);

  a=new QAction(tr("Read only"),this);
  a->setCheckable(true);
  a->setChecked(mandala->readOnly());
  connect(a,SIGNAL(triggered(bool)),mandala,SLOT(setReadOnly(bool)));
  connect(mandala,SIGNAL(readOnlyChanged(bool)),a,SLOT(setChecked(bool)));
  mFile->addAction(a);*/

  /*a=new QAction(tr("Allow external controls"),this);
  a->setCheckable(true);
  a->setChecked(datalink->extctrEnabled());
  QObject::connect(a,SIGNAL(triggered(bool)),datalink,SLOT(setExtctrEnabled(bool)));
  connect(datalink,SIGNAL(extctrEnabledChanged(bool)),a,SLOT(setChecked(bool)));
  mFile->addAction(a);*/

  mServers=new QMenu(tr("Servers"),mFile);
  mServers->setIcon(QIcon(":/icons/old/bt.png"));
  mFile->addMenu(mServers);

  a=new QAction(QIcon(":/icons/old/connect.png"),tr("Connect to..."),this);
  QObject::connect(a,SIGNAL(triggered(bool)),this,SLOT(serverActionConnectTo()));
  mServers->addAction(a);


  mFile->addSeparator();
  mFile->addAction(QIcon(":/icons/old/system-shutdown.png"),tr("Exit"),this,SLOT(close()));


  uavAdded(mandala->current);

  //splash screen setup
  splashScreen = new DockWidget("",this);
  splashScreen->setFloating(false);
  splashScreen->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::TopDockWidgetArea,splashScreen);
  QWidget *splashWidget = new QWidget(this);
  //splashWidget->setStyleSheet("background: url(\":/icons/old/uavos-logo.png\") no-repeat center center fixed;");
  splashScreen->setWidget(splashWidget);
  QVBoxLayout *splashLayout = new QVBoxLayout();
  //splashLayout->setMargin(50);
  splashWidget->setLayout(splashLayout);

  if(QMandala::Global::devMode()){
    QLabel *devLabel=new QLabel(tr("DEVELOPMENT").toUpper(),this);
    devLabel->setAlignment(Qt::AlignCenter);
    devLabel->setFont(QFont("BebasNeue",22));
    splashLayout->addWidget(devLabel);
  }

  QLabel *splashLabel=new QLabel(this);
  splashLabel->setPixmap(QPixmap(":/icons/old/uavos-logo.png"));
  splashLabel->setAlignment(Qt::AlignCenter);
  splashLabel->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
  splashLayout->addWidget(splashLabel);

  loadingLabel=new QLabel(tr("Loading")+"...",this);
  loadingLabel->setAlignment(Qt::AlignCenter);
  splashLayout->addWidget(loadingLabel);

  QString sVer=mandala->version;
  if((!mandala->branch.isEmpty())&&mandala->branch!="master")
    sVer+=" "+mandala->branch.toUpper();
  QLabel *verLabel=new QLabel(sVer,this);
  verLabel->setAlignment(Qt::AlignTop|Qt::AlignRight);
  verLabel->setFont(QFont("FreeMonoBold"));
  splashLayout->addWidget(verLabel);
}
//=============================================================================
MainForm::~MainForm()
{
  //qDeleteAll(plugins.values());
  /*while(shortcuts.keys().size()){
    QShortcut *sc=shortcuts.keys().at(0);
    shortcuts.take(sc);
    delete sc;
  }*/
}
//=============================================================================
void MainForm::closeEvent(QCloseEvent *e)
{
  //save settings
  saveStateTimer.stop();
  QSettings().setValue("maximized", isMaximized());
  QSettings().setValue("fullscreen", isFullScreen());
  saveDockState();

  bool canQuit=!loading;
  foreach(PluginInterface *plugin,plugins_so.values()){
    if(plugin->aboutToQuit())continue;
    canQuit=false;
    break;
  }
  if(canQuit)e->accept();
  else{
    e->ignore();
    return;
  }
  closing=true;
}
//=============================================================================
void MainForm::saveDockState()
{
  QSettings().setValue("state", saveState());
  if(!(isMaximized()||isFullScreen())){
    QSettings().setValue("geometry", geometry());
  }
  //qDebug()<<"saveDockState";
}
//=============================================================================
void MainForm::restoreGeometry()
{
  show();
  if(QSettings().contains("geometry"))setGeometry(QSettings().value("geometry").toRect());
  else resize(QApplication::desktop()->availableGeometry().size()*0.7);

  if(QSettings().value("fullscreen").toBool()) showFullScreen();
  else if(QSettings().value("maximized").toBool()) showMaximized();
}
//=============================================================================
void MainForm::setPlugins(QStringList pfiles)
{
  //QPixmap pixmap(":/icons/old/uavos-logo.png");
  QSplashScreen splash(this);//,pixmap);
  splash.show();

  repaint();
  splashScreen->repaint();
  restoreGeometry();
  splashScreen->repaint();
  update();
  updateGeometry();
  //qApp->processEvents();
  //qApp->processEvents();
  plugins_files=pfiles;
  splash.finish(this);
  loadPlugins();
  //QTimer::singleShot(500,this,SLOT(loadPlugins()));
}
//=============================================================================
void MainForm::loadPlugins()
{
  //load plugin files
  QSettings st;
  st.beginGroup("plugins");
  QStringList loadedNames;
  QList<PluginInterface *>loadedPlugins;
  foreach (QString fileName, plugins_files) {
    QString pname=QFileInfo(fileName).baseName().remove("lib");
    if(loadedNames.contains(pname)){
      qWarning("%s: %s",tr("Duplicate plugin").toUtf8().data(),pname.toUtf8().data());
      continue;
    }
    loadingLabel->setText(QString("%1...").arg(pname).toLower());
    loadingLabel->repaint();

    fprintf(stdout,"%s: %s\n",tr("Loading").toUtf8().data(),pname.toUtf8().data());
    fflush(stdout);
    if(fileName.endsWith(".so")||fileName.endsWith(".dylib")){
      try{
        QPluginLoader loader(fileName);
        if(loader.instance()){
          PluginInterface *plugin=qobject_cast<PluginInterface*>(loader.instance());
          plugins_so[pname]=plugin;
          loadedNames.append(pname);
          loadedPlugins.append(plugin);
          if(!st.contains(pname)) st.setValue(pname,true);
          plugin->name=pname;
          plugin->init();
       }else {
          qWarning("QPluginLoader: %s (%s)",loader.errorString().toUtf8().data(),fileName.toUtf8().data());
        }
      }catch(...){
        qWarning("Plugin load error: %s (%s)",pname.toUtf8().data(),fileName.toUtf8().data());
      }
    }else if(fileName.endsWith(".qml")){
      loadedNames.append(pname);
      if(!st.contains(pname)){
        //defaults to show qml plugins
        st.setValue(pname,pname=="EFIS");
      }
      DockWidget *dock=addDock(pname,NULL);
      dock->toggleViewAction()->setData(QVariant(QStringList()<<pname<<fileName));
      connect(dock->toggleViewAction(),SIGNAL(toggled(bool)),this,SLOT(qmlPluginActionToggled(bool)),Qt::QueuedConnection);
    }
  }
  removeDockWidget(splashScreen);
  //add docks
  foreach(PluginInterface *plugin,loadedPlugins){
    QObject *obj=plugin->obj;
    if(qobject_cast<QAction*>(obj)){
      mTools->addAction(qobject_cast<QAction*>(obj));
    }else if(qobject_cast<QWidget*>(obj)){
      addDock(plugin->name,qobject_cast<QWidget*>(obj));
    }else{
      fprintf(stdout,"%s: %s\n",tr("Feature").toUtf8().data(),plugin->name.toUtf8().data());
    }
  }
  fflush(stdout);

  //read dock geometry state
  QString skey="state";//(isFullScreen()||isMaximized())?"stateMaximized":"state";
  if(QSettings().contains(skey))
    restoreState(QSettings().value(skey).toByteArray());
  else arrange();

  //update window menu
  foreach(DockWidget *dock,plugins_docks.values()){
    mWindow->addAction(dock->toggleViewAction());
  }

  loading=false;
  //emit pluginsLoaded();

  saveStateTimer.setInterval(5000);
  saveStateTimer.setSingleShot(true);
  connect(&saveStateTimer,SIGNAL(timeout()),this,SLOT(saveDockState()));
}
//=============================================================================
void MainForm::dockWidget_viewActionToggled(bool visible)
{
  if(closing)return;
  QString pname=sender()->parent()->objectName();
  QSettings st;
  st.beginGroup("plugins");
  st.setValue(pname,visible);
  //qDebug()<<pname<<visible;
}
void MainForm::qmlPluginActionToggled(bool checked)
{
  //load plugin on first show
  if(!checked)return;
  QAction *a=static_cast<QAction*>(sender());
  QStringList adata=a->data().toStringList();
  QString pname=adata[0];
  if(plugins_qml.contains(pname))return;
  QString fileName=adata[1];
  //load plugin
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();

  /*QmlWidget *qmlWidget=new QmlWidget(QStringLiteral("qrc:///"));
  if(!fileName.startsWith("qrc:"))fileName.prepend("file://");
  qmlWidget->rootContext()->setContextProperty("datalink",datalink);
  qmlWidget->loadApp(fileName);
  plugins_qml.append(pname);
  QDockWidget *dock=static_cast<QDockWidget*>(sender()->parent());
  dock->setWidget(qmlWidget);*/

  /*QmlApp *qmlApp=new QmlApp(QStringLiteral("qrc:///"));
  if(!fileName.startsWith("qrc:"))fileName.prepend("file://");
  qmlApp->rootContext()->setContextProperty("datalink",datalink);
  QQuickWindow *qmlWindow=qmlApp->loadApp(fileName);
  plugins_qml[pname]=qmlApp;
  QDockWidget *dock=static_cast<QDockWidget*>(sender()->parent());
  dock->setWidget(qmlApp->createWidget(a->text()));
  //qmlApp->resyncDraw();*/

  QmlView *view=new QmlView(QStringLiteral("qrc:///"));
  if(!fileName.startsWith("qrc:"))fileName.prepend("file://");
  view->engine()->rootContext()->setContextProperty("datalink",datalink);
  view->setSource(fileName);
  plugins_qml.append(pname);
  QDockWidget *dock=static_cast<QDockWidget*>(sender()->parent());
  dock->setWidget(view->createWidget(a->text()));
  plugins_qmlviews.insert(dock,view); //for focus bugfix
  //qDebug("Plugin loaded: %s",pname.toUtf8().data());
}
//=============================================================================
DockWidget * MainForm::addDock(QString name,QWidget *w)
{
  DockWidget *dock = new DockWidget(w?w->windowTitle():qApp->translate("Plugins",QString(name.left(1).toUpper()+name.mid(1)).toUtf8().data()),this);
  QSize sz=size()*0.38;
  dock->resize(sz.width(),sz.height());
  dock->setObjectName(name);
  dock->setAttribute(Qt::WA_MacAlwaysShowToolWindow,true);
  //dock->setAttribute(Qt::WA_OpaquePaintEvent);
  //dock->setAttribute(Qt::WA_NoSystemBackground);
  //dock->setAttribute(Qt::WA_NoSystemBackground);
  dock->hide();
  dock->setFloating(true);
  dock->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetFloatable);
  //dock->setFeatures(QDockWidget::AllDockWidgetFeatures|QDockWidget::DockWidgetVerticalTitleBar);
  connect(dock, SIGNAL(topLevelChanged(bool)), this, SLOT(dockWidget_topLevelChanged(bool)));
  //connect(dock, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(dockWidget_dockLocationChanged(Qt::DockWidgetArea)));
  connect(dock, SIGNAL(visibilityChanged(bool)), this, SLOT(dockWidget_visibilityChanged(bool)),Qt::QueuedConnection);

  connect(dock, SIGNAL(topLevelChanged(bool)), &saveStateTimer, SLOT(start()));
  connect(dock, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), &saveStateTimer, SLOT(start()));
  connect(dock, SIGNAL(resized()), &saveStateTimer, SLOT(start()));
  addDockWidget(Qt::BottomDockWidgetArea, dock);
  plugins_docks.insertMulti(dock->windowTitle(),dock);
  if(w)dock->setWidget(w);
  return dock;
}
//=============================================================================
bool MainForm::event(QEvent *event)
{ //ACTIVE FOCUS CHANGE QML BUG
  if (event->type() == QEvent::ActivationChange ||
       event->type() == QEvent::WindowUnblocked) {
    //qDebug()<<event;
    foreach(QmlView *view,plugins_qmlviews.values()){
      if(view->isActive()){
        plugins_qmlviews.key(view)->activateWindow();
        return true;
      }
    }
  }
  return QMainWindow::event(event);
}
//=============================================================================
void DockWidget::resizeEvent(QResizeEvent * event)
{
  //qDebug()<<"resizeEvent"<<objectName();
  QDockWidget::resizeEvent(event);
  emit resized();
}
void DockWidget::setDockSize(int w, int h)
{
  oldMaxSize=maximumSize();
  oldMinSize=minimumSize();
  if(w>=0){
    if(width()<w)setMinimumWidth(w);
    else setMaximumWidth(w);
  }
  if(h>=0){
    if(height()<h)setMinimumHeight(h);
    else setMaximumHeight(h);
  }
  QTimer::singleShot(1, this, SLOT(returnToOldMaxMinSizes()));
}
void DockWidget::returnToOldMaxMinSizes()
{
  setMinimumSize(oldMinSize);
  setMaximumSize(oldMaxSize);
}
//=============================================================================
void MainForm::dockWidget_topLevelChanged(bool topLevel)
{
  QDockWidget *dock=static_cast<QDockWidget*>(sender());
  if(topLevel){
    QSize orig=dock->size();
    QSize s=orig;
    QSize max=size()*0.5;
    if(s.width()>max.width())s.setWidth(max.width());
    if(s.height()>max.height())s.setHeight(max.height());
    if(s.height()<50)s.setHeight(50);
    if(s!=orig)dock->resize(s);
  }
  //qDebug()<<"topLevel"<<dock->objectName()<<topLevel;
}
void MainForm::dockWidget_dockLocationChanged(Qt::DockWidgetArea area)
{
  Q_UNUSED(area)
  //qDebug()<<"dockLocationChanged"<<area;
}
void MainForm::dockWidget_visibilityChanged(bool visible)
{
  //QML tabbed dock widget hide workaround
  DockWidget *dw=static_cast<DockWidget*>(sender());
  QWidget *w=dw->widget();
  if(!w)return;
  //qDebug()<<"show: "<<visible<<w->windowTitle();
  if(!visible)return;
  if(w->isFullScreen())return;
  w->hide();
  w->show();
}
//=============================================================================
void MainForm::arrange()
{
  QSettings().remove("geometry");
  QSettings().remove("state");
  restoreGeometry();
  restoreState(QByteArray());
  showMaximized();

  QList<DockWidget *> dockWidgets = findChildren<DockWidget *>();
  QHash<QString,DockWidget*> hash;
  foreach(DockWidget *dock,dockWidgets)
    hash[dock->objectName()]=dock;

  if(hash.contains("EFIS")){
    addDockWidget(Qt::TopDockWidgetArea,hash.value("EFIS"));
    hash.value("EFIS")->setFloating(false);
    hash.value("EFIS")->show();
  }
  if(hash.contains("numbers")){
    addDockWidget(Qt::TopDockWidgetArea,hash.value("numbers"));
    hash.value("numbers")->setFloating(false);
    hash.value("numbers")->show();
  }
  if(hash.contains("joystick")){
    addDockWidget(Qt::TopDockWidgetArea,hash.value("joystick"));
    hash.value("joystick")->setFloating(false);
    hash.value("joystick")->show();
  }

  if(hash.contains("map")){
    addDockWidget(Qt::TopDockWidgetArea,hash.value("map"),Qt::Vertical);
    hash.value("map")->setFloating(false);
    hash.value("map")->show();
    if(hash.contains("telemetry")){
      tabifyDockWidget(hash.value("map"),hash.value("telemetry"));
      hash.value("telemetry")->setFloating(false);
      hash.value("telemetry")->show();
    }
    if(hash.contains("video")){
      tabifyDockWidget(hash.value("map"),hash.value("video"));
      hash.value("video")->setFloating(false);
      hash.value("video")->show();
    }
  }
  if(hash.contains("nodes")){
    addDockWidget(Qt::TopDockWidgetArea,hash.value("nodes"),Qt::Horizontal);
    hash.value("nodes")->setFloating(false);
    hash.value("nodes")->show();
  }

  if(hash.contains("signal")){
    if(hash.contains("nodes")) splitDockWidget(hash.value("nodes"),hash.value("signal"),Qt::Vertical);
    else addDockWidget(Qt::TopDockWidgetArea,hash.value("signal"),Qt::Horizontal);
    hash.value("signal")->setFloating(false);
    hash.value("signal")->show();
  }

  if(hash.contains("console")){
    if(hash.contains("nodes")) splitDockWidget(hash.value("nodes"),hash.value("console"),Qt::Vertical);
    else addDockWidget(Qt::TopDockWidgetArea,hash.value("console"),Qt::Horizontal);
    hash.value("console")->setFloating(false);
    hash.value("console")->show();
  }

  //resize
  QSize ws=size();
  if(hash.contains("EFIS")){
    int w=ws.width()*0.6;
    hash.value("EFIS")->setDockSize(w,w*0.38);
  }

  //qDebug()<<"Auto arrange windows";
  QTimer::singleShot(500, this, SLOT(saveDockState()));
  //saveDockState();
}
//=============================================================================
void MainForm::unlock()
{
  QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
  foreach(QDockWidget *dock,dockWidgets)
    dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
}
//=============================================================================
void MainForm::lock()
{
  QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
  foreach(QDockWidget *dock,dockWidgets)
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
}
//=============================================================================
//=============================================================================
void MainForm::mMandala_triggered()
{
  QString fname=QDir::tempPath()+"/mandala";
  FILE *fd=fopen(fname.toUtf8().data(),"w");
  if (!fd)return;
  mandala->current->print_report(fd);
  fclose(fd);
  QProcess::startDetached("kate",QStringList()<<fname);
}
//=============================================================================
void MainForm::mFullScreen_triggered()
{
  bool bFullScreen=windowState()!=Qt::WindowFullScreen;
  if(bFullScreen)showFullScreen();
  else showNormal();
  QSettings().setValue("fullscreen",bFullScreen);
}
//=============================================================================
void MainForm::mSystem_triggered()
{
  Config *dlg=new Config(this);
  connect(dlg->eAllowExt,SIGNAL(toggled(bool)),datalink,SLOT(setExtctrEnabled(bool)));
  dlg->exec();
  delete dlg;
}
//=============================================================================
void MainForm::mVPN_triggered()
{
  if(vpnProcess.state()==QProcess::Running) vpnProcess.close();
  //login dialog
  QDialog dlg(this);
  dlg.setWindowTitle("UAVOS VPN");
  dlg.setModal(true);
  QGridLayout* formGridLayout = new QGridLayout( &dlg );
  QLineEdit *editUsername = new QLineEdit( QSettings().value("user").toString(),&dlg );
  QLineEdit *editPassword = new QLineEdit( QSettings().value("pass").toString(),&dlg );
  editPassword->setEchoMode( QLineEdit::Password );
  QLabel *labelUsername = new QLabel( &dlg );
  QLabel *labelPassword = new QLabel( &dlg );
  labelUsername->setText( tr( "Username" ) );
  labelUsername->setBuddy( editUsername );
  labelPassword->setText( tr( "Password" ) );
  labelPassword->setBuddy( editPassword );
  QDialogButtonBox *buttons = new QDialogButtonBox( &dlg );
  buttons->addButton( QDialogButtonBox::Ok );
  buttons->addButton( QDialogButtonBox::Cancel );
  buttons->button( QDialogButtonBox::Ok )->setText( tr("Login") );
  connect( buttons->button( QDialogButtonBox::Cancel ),SIGNAL(clicked()),&dlg,SLOT(reject()));
  connect( buttons->button( QDialogButtonBox::Ok ),SIGNAL(clicked()),&dlg,SLOT(accept()) );
  formGridLayout->addWidget( labelUsername, 0, 0 );
  formGridLayout->addWidget( editUsername, 0, 1 );
  formGridLayout->addWidget( labelPassword, 1, 0 );
  formGridLayout->addWidget( editPassword, 1, 1 );
  formGridLayout->addWidget( buttons, 2, 0, 1, 2 );
  dlg.setLayout( formGridLayout );
  if(dlg.exec()!=QDialog::Accepted)return;
  QString user=editUsername->text();
  QString pass=editPassword->text();
  QSettings().setValue("user",user);
  QSettings().setValue("pass",pass);
  //write userpass file
  QFile cf("/tmp/uavos-vpn");
  cf.remove();
  if (!cf.open(QIODevice::WriteOnly|QIODevice::Text))return;
  QTextStream cfs(&cf);
  cfs<<user<<"\n"<<pass;
  cfs.flush();
  cf.flush();
  cf.close();
  //write script file
  QFile sf("/tmp/uavos-vpn.sh");
  sf.remove();
  if (!sf.open(QIODevice::WriteOnly|QIODevice::Text))return;
  sf.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther);
  QTextStream sfs(&sf);
  sfs.setPadChar('\n');
  sfs<<"#!/bin/bash\n";
  QDir vpnp(QMandala::Global::res());
  vpnp.cd("vpn");
  sfs<<"sudo killall -q openvpn\n";
  sfs<<"cd "+vpnp.absolutePath()+"\n";
  sfs<<"sudo openvpn ./client.conf\n";
  sfs.flush();
  sf.flush();
  sf.close();
  //connect
  if(vpnProcess.state()==QProcess::Running){
    qWarning("Can't terminate openvpn process.");
    return;
  }
  QStringList args;
  //args<<"cd "+vpnp.absolutePath()+"; openvpn ./client.conf";
  //QProcess::startDetached("xterm",QStringList()<<"-hold"<<"-e"<<args);
  vpnProcess.start("xterm",QStringList()<<"-e"<<"/tmp/uavos-vpn.sh");
  qDebug("%s",tr("VPN connection initiated").toUtf8().data());
}
void MainForm::vpn_disconnected()
{
  qDebug("%s",tr("VPN disconnected").toUtf8().data());
}
//=============================================================================
void MainForm::mDoc_triggered()
{
  QDesktopServices::openUrl(QUrl("http://wiki.uavos.com"));
}
//=============================================================================
//=============================================================================
//=============================================================================
void MainForm::serverDiscovered(const QHostAddress address,const QString name)
{
  mServers->setEnabled(true);
  QAction *a=new QAction(QIcon(":/icons/old/connect_creating.png"),name,this);
  a->setData(address.toString());
  connect(a,SIGNAL(triggered()),this,SLOT(serverAction()));
  mServers->addAction(a);
}
//=============================================================================
void MainForm::serverAction()
{
  QAction *a=static_cast<QAction*>(sender());
  emit connectToServer(QHostAddress(a->data().toString()));
}
void MainForm::serverActionConnectTo()
{
  QString s=QInputDialog::getText(this,static_cast<QAction*>(sender())->text(),tr("Host address")+":");
  if(s.isNull())return;
  emit connectToServer(QHostAddress(s));
}
//=============================================================================
void MainForm::uavAdded(QMandalaItem *m)
{
  mUAV->setEnabled(mandala->size()>0);
  QAction *a=new QAction(QIcon(":/icons/old/connect_creating.png"),QString(m->ident.callsign),this);
  a->setData(QVariant::fromValue(m));
  connect(a,SIGNAL(triggered()),this,SLOT(uavAction()));
  mUAV->addAction(a);
}
void MainForm::uavAction()
{
  QAction *a=static_cast<QAction*>(sender());
  emit changeUAV(qvariant_cast<QMandalaItem*>(a->data()));
}
//=============================================================================
void MainForm::uavRemoved(QMandalaItem *m)
{
  QList<QAction*>list;
  foreach(QAction *a,mUAV->actions()){
    if(qvariant_cast<QMandalaItem*>(a->data())==m)
      list.append(a);
  }
  foreach(QAction *a,list)mUAV->removeAction(a);
  mUAV->setEnabled(mandala->size()>0);
}
//=============================================================================
//=============================================================================

