#include <QApplication>
#include "QMandala.h"
#include <QmlView.h>
#include "QmlApp.h"
#include "QmlWidget.h"
#include "DatalinkServer.h"
#include "SoundEffects.h"
#include <QQuickStyle>
//=============================================================================
QMandala *mandala;
void loadFonts();
//=============================================================================
int main(int argc, char *argv[])
{
  QGuiApplication::setApplicationName("qgc");
  QGuiApplication::setOrganizationName("uavos.com");
  QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QGuiApplication app(argc, argv);


  //QSettings().setValue("opengl",false);
  //QSettings().setValue("modemHost","");
  //if(!QSettings().contains("readOnly"))QSettings().setValue("readOnly",true);
  //QSettings().setValue("smooth_uav",true);

  /*QFile styleSheet(":img/style-dark.css");
  if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)){
    qApp->setOverrideCursor(Qt::WaitCursor);
    qApp->setStyleSheet(styleSheet.readAll());
    qApp->restoreOverrideCursor();
  }*/


  loadFonts();

  //new MandalaTree();

  mandala=new QMandala();
  mandala->setReadOnly(false);//true);
  mandala->setsoundsEnabled(false);

#ifdef __ANDROID__
  mandala->setsoundsEnabled(true);
#endif


  DatalinkServer *datalink=new DatalinkServer();

  datalink->setHeartbeatEnabled(false);
  datalink->setBindEnabled(false);

  QObject::connect(datalink,SIGNAL(dataReceived(QByteArray)),mandala,SLOT(downlinkReceived(QByteArray)));
  QObject::connect(mandala,SIGNAL(sendUplink(QByteArray)),datalink,SLOT(dataSend(QByteArray)));

  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),mandala->local->rec,SLOT(record_uplink(QByteArray)));

  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),mandala,SLOT(upCntInc()));
  QObject::connect(datalink,SIGNAL(dataReceived(QByteArray)),mandala,SLOT(dlCntInc()));

  datalink->setActive(true);

#ifdef __ANDROID__
  QObject::connect(&app,&QGuiApplication::applicationStateChanged,[=](Qt::ApplicationState state){
    datalink->setActive(state==Qt::ApplicationActive);
  });
#endif

  datalink->setName("qgc");
  datalink->connectToAny();

  QQuickStyle::setStyle("Material");

  QmlView *view=new QmlView(QStringLiteral("qrc:///"));
  new QQmlFileSelector(view->engine(), view);\
  view->engine()->rootContext()->setContextProperty("datalink",datalink);
  view->setSource(QStringLiteral("qrc:///main.qml"));
  QObject::connect(view->engine(), SIGNAL(quit()), qApp, SLOT(quit()));

  /*QmlApp *qmlApp=new QmlApp(QStringLiteral("qrc://"));
  qmlApp->rootContext()->setContextProperty("datalink",datalink);
  qmlApp->loadApp(QStringLiteral("EFIS.qml"),QStringLiteral("Quick Ground Control"));
  QObject::connect(qmlApp, SIGNAL(quit()), qApp, SLOT(quit()));*/

  /*QmlWidget *qmlWidget=new QmlWidget(QStringLiteral("qrc:///"));
  QObject::connect(qmlWidget->engine(), SIGNAL(quit()), qApp, SLOT(quit()));
  qmlWidget->resize(1000,600);
  qmlWidget->rootContext()->setContextProperty("datalink",datalink);
  qmlWidget->loadApp(QStringLiteral("main.qml"));
  qmlWidget->show();*/


  SoundEffects *soundEffects=new SoundEffects(mandala);
  QObject::connect(mandala,SIGNAL(playSoundEffect(QString)),soundEffects,SLOT(play(QString)));


#ifdef __ANDROID__
  //map->showFullScreen();
  view->showFullScreen();
  //qmlApp->qmlWindow->showFullScreen();
#else
  view->show();
  //qmlApp->setGeometry(QRect(100, 100, 1000, 600));
  //qmlApp->show();
#endif
  return app.exec();
}
//=============================================================================
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
