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
#include "QJoystick.h"
#include <linux/input.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <QTimer>
#include "QMandala.h"
#include "QMandala.h"
//=============================================================================
//=============================================================================
Joystick::Joystick(QObject *parent) :
  QObject(parent)
{
  mandala->setJsValid(false);

  cfg=new QSettings(QMandala::Global::config().filePath("joystick.conf"),QSettings::IniFormat);

  scrTimer.setSingleShot(true);
  scrTimer.setInterval(100);
  connect(&scrTimer,SIGNAL(timeout()),this,SLOT(scrTimerTimeout()));

  //open timer
  connect(&timer_open,SIGNAL(timeout()),this,SLOT(scan()));
  timer_open.setInterval(2000);
  timer_open.start();
}
//=============================================================================
void Joystick::scan()
{
  //qDebug()<<"scan";
  QString prefix=cfg->value("prefix","/dev/input/js").toString();
  foreach(const QString &group,cfg->childGroups()){
    QString fname=prefix;
    QString filter;
    if(group.contains(':')){
      fname.append(group.left(group.indexOf(':')));
      if(!QFile::exists(fname))continue;
      filter=group.mid(group.indexOf(':')+1);
      this->open(fname,group,filter);
      continue;
    }
    filter=group;
    for(uint i=0;i<16;i++){
      fname=prefix+QString::number(i);
      if(!QFile::exists(fname))break;
      if(this->open(fname,group,filter))break;
    }
  }
  mandala->setJsValid(jsw_list.size());
}
//=============================================================================
bool Joystick::open(const QString &fname, const QString &group, const QString &filter)
{
  if(jsw_list.contains(fname))return false;
  if(!QFile::exists(fname))return false;
  int fd=::open(fname.toUtf8().data(),O_RDONLY);
  if (fd<0) return false;


  unsigned char caxes = 2;
  unsigned char cbuttons = 2;
  int cversion = 0x000800;
  char cname[128]="Unknown";
  ioctl(fd, JSIOCGVERSION, &cversion);
  ioctl(fd, JSIOCGAXES, &caxes);
  ioctl(fd, JSIOCGBUTTONS, &cbuttons);
  ioctl(fd, JSIOCGNAME(128), cname);

  _jsw j;
  j.name=QString(cname);
  j.fname=fname;
  j.version=cversion;
  j.axes_cnt=caxes;
  j.buttons_cnt=cbuttons;
  j.ax_init=0;

  if(filter.size() && filter!="*" && (!j.name.contains(filter))){
    ::close(fd);
    return false;
  }

  //read conf
  cfg->beginGroup(group);
  foreach(QString key,cfg->childKeys()){
    if(key.trimmed().left(1)=="#")continue;
    if(key.left(3)=="btn"){
      uint i=key.mid(3).toUInt();
      j.buttons[i]=cfg->value(key).toString();
      continue;
    }
    if(key.left(2)=="ax"){
      uint i=key.mid(2).toUInt();
      j.axes[i]=cfg->value(key).toString();
      continue;
    }
  }
  cfg->endGroup();
  if((!j.buttons.size())&&(!j.axes.size())){
    qWarning("joystick: %s",tr("configuration not found").toUtf8().data());
    ::close(fd);
    return false;
  }

  foreach(uint key,j.axes.keys()){
    const QString &cmd=j.axes.value(key);
    if(!cmd.contains("set rc_throttle"))continue;
    //set correction for throttle=0
    struct js_corr corr[128];
    if (ioctl(fd, JSIOCGCORR, &corr)) {
      qWarning("joystick: error getting correction");
      break;
    }
    corr[key-1].coef[0]=128;
    corr[key-1].coef[1]=128;
    if (ioctl(fd, JSIOCSCORR, &corr)){
      qWarning("joystick: error setting correction");
      break;
    }
  }

  j.socketNotifier=new QSocketNotifier(fd,QSocketNotifier::Read);
  connect(j.socketNotifier,SIGNAL(activated(int)),this,SLOT(readEvent(int)));
  qDebug("%s %s: %s",j.fname.mid(11).toUtf8().data(),tr("connected").toUtf8().data(),j.name.toUtf8().data());

  jsw.insert(fd,j);
  jsw_list.append(fname);
  return true;
}
//=============================================================================
void Joystick::close(int fd)
{
  const _jsw &j=jsw.value(fd);
  if(j.socketNotifier){
    j.socketNotifier->setEnabled(false);
    disconnect(j.socketNotifier,SIGNAL(activated(int)));
    delete j.socketNotifier;
  }
  jsw_list.removeAll(j.fname);
  jsw.remove(fd);
  ::close(fd);
  mandala->setJsValid(jsw_list.size());
}
//=============================================================================
//=============================================================================
void Joystick::readEvent(int fd)
{
  _jsw &j=jsw[fd];
  static struct js_event js;
  if (read(fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
    qWarning("%s %s: %s",j.fname.mid(11).toUtf8().data(),tr("disconnected").toUtf8().data(),j.name.toUtf8().data());
    this->close(fd);
    return;
  }

  switch(js.type & ~JS_EVENT_INIT) {
    case JS_EVENT_BUTTON:
      if(js.value!=0){//button pressed
        int i=js.number+1;
        if(j.buttons.contains(i))
          mandala->current->exec_script(j.buttons.value(i));
      }
    break;
    case JS_EVENT_AXIS:{
      if((int)j.ax_init<j.axes_cnt){
        j.ax_init++;
        break;
      }
      int ax=js.value;
      double v=var->limit(ax/32767.0,-1,1);
      int i=js.number+1;
      QString cmd=j.axes.value(i);
      if((cmd.contains("rc_")&&(!cmd.startsWith("jsw_C")))&&(var->status&status_rc))
        break;
      QString key=cmd;
      scrPending[key]=cmd.replace("$",QString("(%1)").arg(v));
      if(!scrTimer.isActive())scrTimer.start();
    }break;
  }
}
//=============================================================================
void Joystick::scrTimerTimeout()
{
  foreach(QString s,scrPending.values())
    mandala->current->exec_script(s);
  scrPending.clear();
}
//==============================================================================

