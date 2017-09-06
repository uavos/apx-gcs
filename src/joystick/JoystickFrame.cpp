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
#include <QtCore>
#include "JoystickFrame.h"
#include "QMandala.h"
#include "position2dwidget.h"
#include "flowlayout.h"
//==============================================================================
JoystickFrame::JoystickFrame(QWidget *parent)
   : QWidget(parent)
{
  setupUi(this);
  toolBar=new QToolBar(this);
  toolBarLayout->addWidget(toolBar);
  toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  toolBar->addAction(aConfig);
  toolBar->addSeparator();
  //toolBar->addAction(aReload);
  //toolBar->addSeparator();

  btnLayout=new FlowLayout(1,1,1);
  ctrWidget->setLayout(btnLayout);

  scrTimer.setSingleShot(true);
  scrTimer.setInterval(100);
  connect(&scrTimer,SIGNAL(timeout()),this,SLOT(scrTimerTimeout()));

  conf=new QSettings(QMandala::Global::config().filePath("joystick.conf"),QSettings::IniFormat);
  QTimer::singleShot(100,aReload,SLOT(trigger()));
}
//==============================================================================
void JoystickFrame::on_aConfig_triggered()
{
  QProcess::startDetached("kate",QStringList()<<(QMandala::Global::config().filePath("joystick.conf")));
}
//==============================================================================
void JoystickFrame::clear()
{
  qDeleteAll(ctrMap.keys());
  ctrMap.clear();
}
//==============================================================================
void JoystickFrame::on_aReload_triggered()
{
  int selIdx=QSettings().value("joystickIndex",-1).toInt();
  cbJsw->clear();
  foreach(QString group,conf->childGroups()){
    if(group.isNull())continue;
    //qDebug()<<group;
    conf->beginGroup(group);
    QString s;
    if(conf->contains("comment")) s=conf->value("comment").toString()+" ("+group+")";
    else s=group;
    cbJsw->addItem(s,group);
    conf->endGroup();
  }
  if(cbJsw->count()>0){
    if(selIdx<0)selIdx=cbJsw->findText("AP control",Qt::MatchContains);
    if(selIdx<0)selIdx=0;
    cbJsw->setCurrentIndex(selIdx);
    on_cbJsw_currentIndexChanged(cbJsw->currentIndex());
  }
}
//==============================================================================
void JoystickFrame::load(QString group)
{
  if(!conf->childGroups().contains(group))return;
  QMap<uint,QString> axes;
  conf->beginGroup(group);
  foreach(QString key,conf->childKeys()){
    if(key.trimmed().left(1)=="#")continue;
    if(key.left(3)=="btn"){
      QString scr=conf->value(key).toString();
      QPushButton *w=new QPushButton(caption(scr));
      w->setObjectName("jswButton");
      w->setToolTip(scr);
      connect(w,SIGNAL(clicked()),this,SLOT(buttonClicked()));
      ctrMap.insert(w,scr);
      btnLayout->addWidget(w);
      continue;
    }
    if(key.left(2)=="ax"){
      uint i=key.mid(2).toUInt();
      axes.insert(i,conf->value(key).toString());
      continue;
    }
  }
  conf->endGroup();
  //add axes
  if(axes.contains(1)&&axes.contains(2)){
    Position2dWidget *w=new Position2dWidget();
    ctrMap.insert(w,axes.value(1)+":"+axes.value(2));
    ctrLayout->addWidget(w);
    w->setToolTip(axes.value(1)+"\n"+axes.value(2));
    w->setEnabledXMoving(true);
    w->setEnabledYMoving(true);
    connect(w,SIGNAL(XChanged(double)),this,SLOT(XChanged(double)));
    connect(w,SIGNAL(YChanged(double)),this,SLOT(YChanged(double)));
    w->show();
    axes.remove(1);
    axes.remove(2);
  }
  foreach(QString cmd,axes.values()){
    QSlider *w=new QSlider(Qt::Horizontal);
    w->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    w->setMaximumWidth(Position2dWidget::WIDGET_WIDTH);
    w->setToolTip(cmd);
    connect(w,SIGNAL(valueChanged(int)),this,SLOT(sliderMoved(int)));
    ctrMap.insert(w,cmd);
    ctrLayout->addWidget(w);
    w->show();
  }
}
//==============================================================================
void JoystickFrame::on_cbJsw_currentIndexChanged(int index)
{
  clear();
  if(index<0)return;
  load(cbJsw->itemData(index).toString());
  QSettings().setValue("joystickIndex",index);
}
//==============================================================================
QString JoystickFrame::caption(QString cmd)
{
  if(cmd.startsWith("set")){
    QString s=cmd.mid(cmd.indexOf('(')+1);
    s=s.left(s.indexOf(')'));
    QString sn=s.left(s.indexOf(',')).remove("\'");
    QString sv=s.mid(s.indexOf(',')+1);
    if(sn.isNull()||sv.isNull())return cmd;
    if(sn=="mode")return sv;
    if(sv.startsWith("trigger")||sv.startsWith('('))return sn;
    sv.remove('!');
    return sv;
  }else if(cmd.contains('=')){
    QString sn=cmd.left(cmd.indexOf('='));
    QString sv=cmd.mid(cmd.indexOf('=')+1);
    if(sn.isNull()||sv.isNull())return cmd;
    if(sn=="mode")return sv.mid(sv.indexOf('_')+1);
    if(sv.startsWith("trigger")||sv.startsWith('('))sv=sn;
    sv.remove('!');
    if(sv.startsWith("ctr_")||sv.startsWith("ctrb_")||sv.startsWith("sw_")||sv.startsWith("cmode_"))
      sv.remove(0,sv.indexOf('_')+1);
    return sv;
  }else if(cmd.contains('(')){
    QString sv=cmd.left(cmd.indexOf('('));
    if(sv.contains('_'))sv=sv.mid(sv.indexOf('_')+1);
    return sv;
  }
  return cmd;
}
//==============================================================================
void JoystickFrame::buttonClicked()
{
  if(!ctrMap.contains(static_cast<QWidget*>(sender())))return;
  var->exec_script(ctrMap.value(static_cast<QWidget*>(sender())));
}
//==============================================================================
void JoystickFrame::sliderMoved(int v)
{
  if(!ctrMap.contains(static_cast<QWidget*>(sender())))return;
  double vf=2.0*v/100.0-1.0;
  QString s=ctrMap.value(static_cast<QWidget*>(sender()));
  //if(s.contains("rc_throttle")||s.contains("jsw_T"))vf=-vf;
  vf=-vf;
  QString key=s;
  scrPending[key]=s.replace("$",QString("(%1)").arg(vf));
  if(!scrTimer.isActive())scrTimer.start();
}
//==============================================================================
void JoystickFrame::XChanged(double x)
{
  if(!ctrMap.contains(static_cast<QWidget*>(sender())))return;
  QString s=ctrMap.value(static_cast<QWidget*>(sender()));
  s=s.left(s.indexOf(':'));
  QString key=s;
  scrPending[key]=s.replace("$",QString("(%1)").arg(x));
  if(!scrTimer.isActive())scrTimer.start();
}
//==============================================================================
void JoystickFrame::YChanged(double y)
{
  if(!ctrMap.contains(static_cast<QWidget*>(sender())))return;
  QString s=ctrMap.value(static_cast<QWidget*>(sender()));
  s=s.mid(s.indexOf(':')+1);
  QString key=s;
  scrPending[key]=s.replace("$",QString("(%1)").arg(-y));
  if(!scrTimer.isActive())scrTimer.start();
}
//==============================================================================
void JoystickFrame::scrTimerTimeout()
{
  foreach(QString s,scrPending.values())
    var->exec_script(s);
  scrPending.clear();
}
//==============================================================================

