#include <QCoreApplication>
#include "QMandala.h"
#include "QMandala.h"
#include "Serial.h"
#include "DatalinkServer.h"
#include "HttpService.h"
//============================================================================
//global variables
Serial *serial1;
//============================================================================
int main(int argc, char *argv[])
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setOrganizationName("uavos.com");
  QCoreApplication::setOrganizationDomain("www.uavos.com");
  QCoreApplication::setApplicationName("gcu-server");
  QSettings().setDefaultFormat(QSettings::IniFormat);

  //parse command line arguments
  QSettings().setValue("serial1","auto");
  QSettings().setValue("serial1_baudrate",460800);
  QStringListIterator args(QCoreApplication::arguments());
  while(args.hasNext()){
    QString s=args.next();
    s=s.trimmed();
    if(!s.size())continue;
    if(s.at(0)!='-')continue;
    if(s=="-d"){
      if(!args.hasNext())break;
      s=args.next();
      QSettings().setValue("serial1",s);
      qDebug("Serial port: %s",s.toUtf8().data());
      continue;
    }
    if(s=="-b"){
      if(!args.hasNext())break;
      s=args.next();
      QSettings().setValue("serial1_baudrate",s);
      qDebug("Baud rate: %s",s.toUtf8().data());
      continue;
    }
    if(s=="-h"){
      qDebug("Options:");
      qDebug(" -d /dev/ttyS0|auto\tset serial port");
      qDebug(" -b 460800\t\tset baud rate");
      return 0;
    }
  }


  //create main objects

  DatalinkServer *datalink=new DatalinkServer;

  serial1=new Serial(0,NULL,true);
  QObject::connect(serial1,SIGNAL(received(QByteArray)),datalink,SLOT(localDataReceived(QByteArray)));
  QObject::connect(datalink,SIGNAL(loacalDataSend(QByteArray)),serial1,SLOT(send(QByteArray)));
  QObject::connect(datalink,SIGNAL(heartbeat(QByteArray)),serial1,SLOT(send(QByteArray)));

  datalink->activate();

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //  exec
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  int rv=app.exec();
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  serial1->close();

  return rv;
}
//============================================================================
