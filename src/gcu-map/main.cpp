#include <QApplication>
#include "QMandala.h"
#include <QmlView.h>
#include "DatalinkServer.h"
#include "QmlMap.h"
#include "QmlMapPath.h"
#include "MissionPath.h"
#include "QmlMissionModel.h"
//=============================================================================
QMandala *mandala;
void loadFonts();
//=============================================================================
int main(int argc, char *argv[])
{
  QApplication application(argc, argv);

  QFile styleSheet(":styles/style-dark.css");
  if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)){
    qApp->setOverrideCursor(Qt::WaitCursor);
    qApp->setStyleSheet(styleSheet.readAll());
    qApp->restoreOverrideCursor();
  }


  loadFonts();

  QMandala *mandala=new QMandala();
  mandala->setReadOnly(false);//true);
  mandala->setsoundsEnabled(false);

  DatalinkServer *datalink=new DatalinkServer();

  datalink->setHeartbeatEnabled(false);
  datalink->setBindEnabled(false);

  QObject::connect(datalink,SIGNAL(dataReceived(QByteArray)),mandala,SLOT(downlinkReceived(QByteArray)));
  QObject::connect(mandala,SIGNAL(sendUplink(QByteArray)),datalink,SLOT(dataSend(QByteArray)));

  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),QMandala::instance()->current->rec,SLOT(record_uplink(QByteArray)));

  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),mandala,SLOT(upCntInc()));
  QObject::connect(datalink,SIGNAL(dataReceived(QByteArray)),mandala,SLOT(dlCntInc()));

  datalink->setActive(true);

  datalink->setName("map");
  datalink->connectToAny();

  QmlView *view=new QmlView(QStringLiteral("qrc:///"));

  view->engine()->rootContext()->setContextProperty("datalink",datalink);

  QSurfaceFormat format = view->format();
  format.setSamples(16);
  view->setFormat(format); //antialiased geometry

  QmlMap::registerTypes();

  //new MissionModelQml(view);

  view->setSource(QStringLiteral("qrc:///MapView.qml"));

  QObject::connect(view->engine(), SIGNAL(quit()), qApp, SLOT(quit()));


  view->setGeometry(QRect(100, 100, 1000, 600));
  view->show();
  return application.exec();
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
