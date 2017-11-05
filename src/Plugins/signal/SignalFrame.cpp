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
#include <Vehicles>
#include "SignalFrame.h"
//==============================================================================
SignalFrame::SignalFrame(QWidget *parent) :
    QWidget(parent)
{
  setupUi(this);
  toolBar=new QToolBar(this);
  toolBar->setObjectName("toolBarSW");
  toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
  toolBar->layout()->setMargin(0);
  toolBar->layout()->setSpacing(0);
  toolBarLayout->insertWidget(0,toolBar);
  actionGroup=new QActionGroup(this);

  plot->setStackGraph(false);
  plot->setFillOpacity(0);

  plot->setHorizontalScale(2);
  plot->setShowAxis(true);
  plot->setMaxAxisTextWidth(22);
  QFont fnt=font();
  fnt.setPixelSize(8);
  plot->setAxisFont(fnt);

  addPlot("R", QList<QColor>()<<QColor(Qt::red).lighter()<<Qt::red, QList<uint>()<<(idx_theta|0x0000)<<(idx_cmd_theta|0x0000));
  addPlot("P", QList<QColor>()<<QColor(Qt::green)<<Qt::darkGreen, QList<uint>()<<(idx_theta|0x0100)<<(idx_cmd_theta|0x0100));
  addPlot("Y", QList<QColor>()<<QColor(Qt::yellow)<<Qt::darkYellow<<Qt::lightGray, QList<uint>()<<(idx_theta|0x0200)<<(idx_cmd_theta|0x0200)<<idx_cmd_course);
  addPlot("Axy", QList<QColor>()<<QColor(Qt::red).lighter()<<QColor(Qt::green).lighter(), QList<uint>()<<(idx_acc)<<(idx_acc|0x0100));
  addPlot("Az", QList<QColor>()<<QColor(Qt::blue).lighter(), QList<uint>()<<(idx_acc|0x0200));
  addPlot("G", QList<QColor>()<<Qt::red<<Qt::green<<Qt::blue, QList<uint>()<<(idx_gyro|0x0000)<<(idx_gyro|0x0100)<<(idx_gyro|0x0200));
  addPlot("M", QList<QColor>()<<Qt::red<<Qt::green<<Qt::blue, QList<uint>()<<(idx_mag|0x0000)<<(idx_mag|0x0100)<<(idx_mag|0x0200));
  addPlot("Pt", QList<QColor>()<<Qt::red<<Qt::green<<Qt::blue, QList<uint>()<<idx_altitude<<idx_vspeed<<idx_airspeed);
  addPlot("Ctr", QList<QColor>()
  <<QColor(Qt::red).lighter()
  <<QColor(Qt::green).lighter()
  <<QColor(Qt::blue).lighter()
  <<QColor(Qt::yellow).lighter()
  <<Qt::darkCyan
  <<Qt::red
  <<Qt::darkGreen
  <<Qt::darkBlue
  <<Qt::darkYellow
  ,QList<uint>()
  <<idx_ctr_ailerons
  <<idx_ctr_elevator
  <<idx_ctr_throttle
  <<idx_ctr_rudder
  <<idx_ctr_collective
  <<idx_rc_roll
  <<idx_rc_pitch
  <<idx_rc_throttle
  <<idx_rc_yaw
  );
  addPlot("Usr", QList<QColor>()
          <<Qt::red<<Qt::green<<Qt::blue<<Qt::yellow<<Qt::cyan<<Qt::magenta
  ,QList<uint>()
          <<idx_user1<<idx_user2<<idx_user3<<idx_user4<<idx_user5<<idx_user6);

  toolBar->addSeparator();
  QAction *a=map.keys().first();
  foreach(QAction *ai,map.keys())
    if(QSettings().value("signal_plot").toString()==ai->text()){
      a=ai;
      break;
    }
  a->toggle();

  connect(Vehicles::instance(),&Vehicles::currentDataReceived,this,&SignalFrame::dataReceived);
}
//==============================================================================
QAction* SignalFrame::addPlot(QString name,const QList<QColor> &color,const QList<uint> &var_idx)
{
  //button
  QAction *a=toolBar->addAction(name);
  a->setCheckable(true);
  QString s="<html><NOBR>";
  int cnt=0;
  foreach(uint i,var_idx){
    QColor c=color.at(cnt++);
    QString cn;
    for(int ci=0;ci<QColor::colorNames().size();ci++){
      cn=QColor::colorNames().at(ci);
      if(c==QColor(cn))break;
      cn=c.name();
    }
    s+="<b><font color="+cn+">"+cn.toUpper()+": </font></b>"+QString(Vehicles::instance()->current()->f_mandala->factById(i)->descr())+"<br>\n";
  }
  a->setToolTip(s);
  actionGroup->addAction(a);
  connect(a,SIGNAL(toggled(bool)),SLOT(action_toggled(bool)));
  //map
  _plot b;
  b.var_idx=var_idx;
  b.color=color;
  map.insert(a,b);
  return a;
}
//==============================================================================
void SignalFrame::dataReceived(uint id)
{
  if(id!=idx_downstream)return;
  QList<double> list;
  if(!map.contains(curAction))return;
  const _plot &b=map.value(curAction);
  foreach(uint var_idx,b.var_idx)
    list.append(Vehicles::instance()->current()->f_mandala->valueById(var_idx).toDouble());
  plot->addSample(list);
  plot->setUseAutoRange(true);
  plot->update();
  //QTimer::singleShot(100,this,SLOT(plotAppend()));
}
//==============================================================================
void SignalFrame::action_toggled(bool checked)
{
  if(!checked)return;
  curAction=(QAction*)sender();
  while(plot->numBeams()) plot->removeBeam(0);
  if(!map.contains(curAction))return;
  const _plot &b=map.value(curAction);
  foreach(QColor c,b.color)
    plot->addBeam(c);
  plot->changeRange(-1,1);
  //plot->setUseAutoRange(true);
  QSettings().setValue("signal_plot",curAction->text());
}
//==============================================================================
