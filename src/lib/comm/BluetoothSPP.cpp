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
#include "BluetoothSPP.h"
//=============================================================================
BluetoothSPP::BluetoothSPP(int num, QObject * parent, bool active)
  : QObject(parent),
    num(num)
{
  thr=NULL;
  scanIdx=0;
  if(active)QTimer::singleShot(1000, this, SLOT(open()));
}
//=============================================================================
void BluetoothSPP::activate()
{
  open();
}
//=============================================================================
bool BluetoothSPP::open()
{
 if (isOpen()){
    socketNotifier=new QSocketNotifier(uart.handle(),QSocketNotifier::Read);
    connect(socketNotifier,SIGNAL(activated(int)),this,SLOT(read()));//,Qt::QueuedConnection);
    if(num<0){
      qDebug("%s: %s",tr("BluetoothSPP port connected").toUtf8().data(),pname.toUtf8().data());
    }else{
      qDebug("%s: %s (#%u)",tr("BluetoothSPP port connected").toUtf8().data(),pname.toUtf8().data(),num+1);
    }
    return true;
  }
  pname="/dev/rfcomm0";
  if(pname=="auto"){
    //find next port dev
    pname.clear();
    QString prefix("ttyUSB");
    QDir dev_dir("/dev",prefix+"*",QDir::Name,QDir::System|QDir::Writable|QDir::Files);
    QStringList dev_list=dev_dir.entryList();
    for(int i=0;i<100;i++){
      QString s=QString("%1%2").arg(prefix).arg(scanIdx);
      if(dev_list.contains(s)){
        QString fname=dev_dir.absoluteFilePath(s);
        if(uart.isAvailable(fname.toUtf8().data())){
          pname=fname;
          break;
        }
      }
      scanIdx++;
      if(scanIdx>99)scanIdx=0;
    }
  }
  if(!(pname.startsWith('/') && QFile::exists(pname))){
    QTimer::singleShot(1000, this, SLOT(open()));
    return false;
  }
  tryOpen(pname,460800);
  return false;
}
//=============================================================================
void BluetoothSPP::close(void)
{
  if(isOpen())uart.close();
}
//=============================================================================
void BluetoothSPP::read()
{
  if(!isOpen())return;
  do{
    uint cnt=uart.readEscaped(rxbuf,sizeof(rxbuf));
    if(cnt>0){
      QByteArray ba((const char*)rxbuf,cnt);
      emit received(QByteArray((const char*)rxbuf,cnt));
      timeout.start();
      //break;
      continue;
    }
    if(QFile::exists(pname))continue;
    if(timeout.elapsed()<1000)return;
    if(num<0){
      qWarning("%s",tr("BluetoothSPP port disconnected.").toUtf8().data());
    }else{
      qWarning("%s (#%u)",tr("BluetoothSPP port disconnected.").toUtf8().data(),num+1);
    }
    disconnect(socketNotifier,SIGNAL(activated(int)));
    delete socketNotifier;
    uart.close();
    QTimer::singleShot(1000, this, SLOT(open()));
    return;
  }while(uart.getRxCnt());
}
//=============================================================================
void BluetoothSPP::send(const QByteArray &ba)
{
  if(!isOpen())return;
  //qDebug()<<ba.toHex();
  uart.writeEscaped((uint8_t*)ba.data(),ba.size());
}
//=============================================================================
bool BluetoothSPP::isOpen(void)
{
  return (!thr)&&uart.isOpen();
}
//=============================================================================
void BluetoothSPP::tryOpen(QString name, uint baudrate)
{
  thr=new tryOpenThread(this,&uart,name,baudrate);
  connect(thr, SIGNAL(finished()), this, SLOT(openFinished()),Qt::QueuedConnection);
  thr->start(QThread::LowPriority);
}
//=============================================================================
void BluetoothSPP::openFinished()
{
  bool rv=thr->result;
  //thr->quit();
  disconnect(thr, SIGNAL(finished()), this, SLOT(openFinished()));
  thr->deleteLater();
  thr=NULL;
  QTimer::singleShot(rv?200:1000, this, SLOT(open()));
}
//=============================================================================
//=============================================================================
