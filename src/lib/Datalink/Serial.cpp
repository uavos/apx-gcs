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
#include "Serial.h"
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/*#ifdef __APPLE__
#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#include <IOKit/serial/ioss.h>
#endif
#endif*/
//=============================================================================
Serial::Serial(int num,QObject * parent,bool active)
  : QObject(parent)
{
  worker=new SerialWorker(num);
  worker->moveToThread(&thr);
  connect(&thr, &QThread::finished, worker, &QObject::deleteLater);
  connect(this, &Serial::w_activate, worker, &SerialWorker::activate);
  connect(this, &Serial::w_close, worker, &SerialWorker::closePort);//,Qt::BlockingQueuedConnection);
  connect(this, &Serial::w_send, worker, &SerialWorker::send,Qt::QueuedConnection);
  connect(worker, &SerialWorker::received, this, &Serial::w_received,Qt::BlockingQueuedConnection);
  thr.start();
  if(active)QTimer::singleShot(2000,this,SIGNAL(w_activate()));
}
void Serial::activate()
{
  emit w_activate();
}
void Serial::close()
{
  emit w_close();
}
void Serial::send(const QByteArray ba)
{
  emit w_send(ba);
}
void Serial::w_received(QByteArray ba)
{
  emit received(ba);
}
Serial::~Serial()
{
  emit w_close();
  thr.quit();
  thr.wait();
}
//=============================================================================
SerialWorker::SerialWorker(int num)
  : QSerialPort(), _escaped(),
    num(num), o_brate(0)
{
  m_open=false;
  scan_idx=0;

  socketNotifier=NULL;

  txba.reserve(8192);

  fd=-1;

  sp=this;//new QSerialPort(0);
  //sp->moveToThread(thread);

  //default settings
  if (!settings.contains("serial1_baudrate"))settings.setValue("serial1_baudrate","460800");
  if (!settings.contains("serial2_baudrate"))settings.setValue("serial2_baudrate","460800");
  if(num<=0){
    if (!settings.contains("serial1"))settings.setValue("serial1","auto");
  }

  connect(sp,SIGNAL(readyRead()),this,SLOT(newDataAvailable()));
  connect(sp,SIGNAL(errorOccurred(QSerialPort::SerialPortError)),this,SLOT(serialError(QSerialPort::SerialPortError)));

  openTimer=NULL;
  //open port scanner
  //if(active)openTimer->start(2000);
}
SerialWorker::~SerialWorker()
{
  closePort();
  //delete sp;
}
//=============================================================================
QStringList SerialWorker::openPorts=QStringList();
//=============================================================================
void SerialWorker::activate()
{
  if(!openTimer){
    openTimer=new QTimer(this);
    openTimer->setSingleShot(true);
    openTimer->setInterval(800);
    connect(openTimer,&QTimer::timeout,this,&SerialWorker::tryOpen);
  }
  openTimer->start();
}
//=============================================================================
bool SerialWorker::isOpen(void)
{
  return m_open;
}
//=============================================================================
void SerialWorker::tryOpen()
{
  if (isOpen()) return;
  const QString & srkey=QString("serial%1").arg(num+1);
  QString pname=o_pname.isEmpty()?settings.value(srkey).toString().trimmed():o_pname;

  while(1){
    QSerialPortInfo spi;
    if((!pname.isEmpty()) && pname!="auto"){
      //the port name is specified
      spi=QSerialPortInfo(pname);
    }else{
      //[auto] - scan for any next available port
      QList<QSerialPortInfo> list=QSerialPortInfo::availablePorts();
      if(list.isEmpty())break;
      if(scan_idx>=list.size())scan_idx=0;
      for(int i=0;i<list.size();i++){
        QSerialPortInfo lspi=list.at(scan_idx);
        if(lspi.portName().contains("USB",Qt::CaseInsensitive) && lspi.portName().contains("tty",Qt::CaseInsensitive) && isAvailable(lspi)){
          spi=lspi;
          break;
        }
        scan_idx++;
        if(scan_idx>=list.size())scan_idx=0;
      }
    }
    if(!isAvailable(spi))break;
    //try to open valid spi port
    //qDebug("Trying to open %s (%s)",spi.portName().toUtf8().data(),spi.systemLocation().toUtf8().data());
    if(!openPort(spi,o_brate?o_brate:settings.value(srkey+"_baudrate",460800).toUInt())){
      qDebug("Serial port open failed (%s)",spi.portName().toUtf8().data());
      break;
    }
    portInfo=spi;
    openPorts.append(portInfo.portName());
    if(num<0){
      qDebug("%s: %s",tr("Serial port connected").toUtf8().data(),portInfo.portName().toUtf8().data());
    }else{
      qDebug("%s: %s (#%u)",tr("Serial port connected").toUtf8().data(),portInfo.portName().toUtf8().data(),num+1);
    }
    m_open=true;
    return;
  }
  //continue to watch for ports to be available
  //openTimer->moveToThread(this->thread());
  /*if(!openTimer){
    openTimer=new QTimer(this);
    openTimer->setSingleShot(true);
    openTimer->setInterval(800);
    connect(openTimer,&QTimer::timeout,this,&SerialWorker::tryOpen);
  }
  openTimer->start();*/
  activate();
}
//=============================================================================
bool SerialWorker::isAvailable(const QSerialPortInfo &spi)
{
  if(spi.isNull())return false;
  if(openPorts.contains(spi.portName()))return false;

  /*const char *portname=spi.systemLocation().toUtf8().data();
  int fdx = ::open(portname, O_RDWR | O_NOCTTY );
  if(fdx<0) return false;
  if(::flock(fdx,LOCK_EX|LOCK_NB)!=0){
    ::close(fdx);
    return false;
  }
  ::flock(fdx,LOCK_UN);
  ::close(fdx);*/
  return true;
}
bool SerialWorker::openPort(const QSerialPortInfo &spi, int baudrate)
{
  sp->setPort(spi);
  sp->setBaudRate(baudrate);
  return sp->open(QIODevice::ReadWrite);

  /*const char *portname=spi.systemLocation().toUtf8().data();

  fd = ::open(portname, O_RDWR | O_NOCTTY );// | O_NONBLOCK | O_NDELAY);
  if (fd <0) {
    fprintf(stderr,"Unable to open %s - %s\n",portname,strerror(errno));
    return false;
  }
  if(::flock(fd,LOCK_EX|LOCK_NB)!=0){
    closePort();
    fprintf(stderr,"Unable to lock %s - %s\n",portname,strerror(errno));
    return false;
  }

  uint parity=0;

  struct termios tio_serial;
  bzero(&tio_serial, sizeof(tio_serial));
  tio_serial.c_cflag = CS8 | CLOCAL | CREAD | parity;
  tio_serial.c_iflag = IGNBRK | (parity?INPCK:IGNPAR);
  tio_serial.c_oflag = 1;
  tio_serial.c_lflag = 0;
  tio_serial.c_cc[VMIN] = 0;
  tio_serial.c_cc[VTIME] = 1;

#ifdef __APPLE__
  if (::ioctl(fd, IOSSIOSPEED, &baudrate) == -1) {
    fprintf(stderr,"Unable to set baudrate %s - %s\n",portname,strerror(errno));
    return false;
  }
#else
  cfsetspeed(&tio_serial, baudrate);
#endif

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &tio_serial);

  socketNotifier=new QSocketNotifier(fd,QSocketNotifier::Read);
  connect(socketNotifier,SIGNAL(activated(int)),this,SLOT(newDataAvailable()));//,Qt::QueuedConnection);
  return true;*/
}
void SerialWorker::closePort(void)
{
  if(isOpen()) sp->close();
  m_open=false;
  if(openTimer)openTimer->stop();
  /*if(fd>=0){
    //sp->close();
    ::flock(fd,LOCK_UN);
    ::close(fd);
    fd=-1;
  }
  if(socketNotifier){
    disconnect(socketNotifier,SIGNAL(activated(int)));
    delete socketNotifier;
    socketNotifier=NULL;
  }*/

  openPorts.removeAll(portInfo.portName());
  //fprintf(stdout,"\nCLOSE PORT\n");
}
//=============================================================================
void SerialWorker::serialError(QSerialPort::SerialPortError error)
{
  if(error==QSerialPort::NoError)return;
  //qWarning("Serial error: %i",error);
  errClose();
}
//=============================================================================
void SerialWorker::newDataAvailable()
{
  if(!isOpen())return;
  do{
    uint cnt=readEscaped();
    if(cnt>0){
      emit received(QByteArray((const char*)esc_rx,cnt));
    }
  }while(isOpen() && getRxCnt()>0);//sp->bytesAvailable());
}
//=============================================================================
void SerialWorker::send(const QByteArray ba)
{
  if(!isOpen())return;
  //qDebug()<<ba.toHex();
  writeEscaped((const uint8_t*)ba.data(),ba.size());
}
//=============================================================================
unsigned int SerialWorker::getRxCnt(void)
{
  return sp->bytesAvailable();
  /*unsigned int cnt;
  ::ioctl(fd, FIONREAD, &cnt);
  return cnt;*/
}
//==============================================================================
//=============================================================================
uint SerialWorker::esc_read(uint8_t *buf,uint sz)
{
  if(!isOpen())return 0;
  int cnt=sp->read((char*)buf,sz);
  //int cnt=::read(fd,buf,sz);
  if(cnt>=0)return cnt;
  //read error
  errClose();
  return 0;
}
bool SerialWorker::esc_write_byte(const uint8_t v)
{
  txba.append((char)v);
  return true;
}
void SerialWorker::escWriteDone(void)
{
  if(txba.isEmpty())return;
  if(isOpen()){
    if(sp->write(txba)<=0) errClose();
    /*int pcnt=txba.size();
    int wcnt=0;
    const uint8_t *buf=(const uint8_t *)txba.data();
    while (wcnt<pcnt) {
      int rcnt=::write(fd,buf+wcnt,pcnt-wcnt);
      if(rcnt<=0){
        errClose();
        break;
      }
      wcnt+=rcnt;
    }*/
  }
  txba.clear();
}
//=============================================================================
//=============================================================================
void SerialWorker::errClose()
{
  if(!isOpen()) return;
  if(num<0){
    qWarning("%s",tr("Serial port disconnected.").toUtf8().data());
  }else{
    qWarning("%s (#%u)",tr("Serial port disconnected.").toUtf8().data(),num+1);
  }
  closePort();
  m_open=false;
  openTimer->start();
}
//=============================================================================
