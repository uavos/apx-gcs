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
#include "BlackboxDownload.h"
#include "QMandala.h"
#include "NodesModel.h"
#include "node.h"
#include "crc.h"
//=============================================================================
BlackboxDownload::BlackboxDownload(NodesItemNode *node, QWidget *parent) :
    QWidget(parent),node(node)
{
  setupUi(this);
  setObjectName(node->name+"_bbdownload");
  restoreGeometry(QSettings().value(objectName()).toByteArray());

  QToolBar *toolBar=new QToolBar(this);
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBar->setIconSize(QSize(16,16));
  toolBar->layout()->setMargin(0);
  toolBarLayout->insertWidget(0,toolBar);
  toolBar->addAction(aRefresh);
  toolBar->addAction(aRead);
  toolBar->addSeparator();
  toolBar->addAction(aErase);
  toolBar->addAction(aAbort);

  aAbort->setEnabled(false);
  aRead->setEnabled(false);
  aErase->setEnabled(false);

  mvar=new QMandalaItem(this,true);

  int icmd=node->commands.name.indexOf("bb_read");
  bb_read=icmd>=0?node->commands.cmd.at(icmd):0;
  icmd=node->commands.name.indexOf("bb_erase");
  bb_erase=icmd>=0?node->commands.cmd.at(icmd):0;
  stage=stage_idle;
  do_stop=false;
  retry=0;

  timer.setSingleShot(true);
  timer.setInterval(500);
  connect(&timer,SIGNAL(timeout()),this,SLOT(next()));

  lbNode->setText(QString("%1-%2").arg(node->name).arg(node->valueByName("comment").toString()));
  eUAV->setText(QString("%1-%2").arg(QMandala::instance()->uavName()).arg(lbNode->text()));

  progressBar->setVisible(false);

  connect(&reader,SIGNAL(packet_read(QByteArray)),mvar,SLOT(downlinkReceived(QByteArray)));

  //connect(ui->btnReset,SIGNAL(pressed()),this,SLOT(btnReset()));
  //connect(ui->btnSend,SIGNAL(pressed()),this,SLOT(btnSend()));
  //connect(ui->btnForward,SIGNAL(pressed()),this,SLOT(btnForward()));
  //connect(ui->eTxText,SIGNAL(returnPressed()),this,SLOT(btnSend()));

  //ui->ePortID->setValue(QSettings().value(objectName()+"_port").toUInt());
  //ui->eForward->setText(QSettings().value(objectName()+"_fwdDev").toString());
  //connect(mandala,SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(mandalaCurrentChanged(QMandalaItem*)));
  //mandalaCurrentChanged(mandala->current);
  connect(killButton, SIGNAL(clicked()), this, SLOT(close()));
  next();
  QTimer::singleShot(500,aRefresh,SLOT(trigger()));
  //aRefresh->trigger();
}
//==============================================================================
BlackboxDownload::~BlackboxDownload()
{
  delete ui;
}
//=============================================================================
void BlackboxDownload::closeEvent(QCloseEvent *event)
{
  Q_UNUSED(event)
  QSettings().setValue(objectName(),saveGeometry());
  //QSettings().setValue(objectName()+"_port",ui->ePortID->value());
  emit finished();
}
//=============================================================================
//=============================================================================
void BlackboxDownload::next()
{
  timer.stop();
  if(do_stop){
    do_stop=false;
    stage=stage_idle;
  }
  switch(stage){
    default: //idle
      retry=0;
      progressBar->setVisible(false);
      aRead->setEnabled(hdr.rec_size>0);
      aErase->setEnabled(hdr.rec_size>0);
      aRefresh->setEnabled(true);
      aAbort->setEnabled(false);
      mvar->rec->setRecording(false);
      mvar->rec->flush();
      return;
    case stage_init:
      if((++retry)>4)break;
      emit request(bb_read,node->sn,QByteArray(),0);
      //qDebug("stage_init");
      break;
    case stage_read: {
      if((++retry)>4)break;
      QByteArray ba;
      ba.append((const char)req_blk&0xFF);
      ba.append((const char)(req_blk>>8)&0xFF);
      ba.append((const char)(req_blk>>16)&0xFF);
      ba.append((const char)(req_blk>>24)&0xFF);
      emit request(bb_read,node->sn,ba,0);
      aAbort->setEnabled(true);
    } break;
    case stage_erase:
      emit request(bb_erase,node->sn,QByteArray(),0);
      stage=stage_init;
      break;
  }
  if(retry>1){
    qWarning("Blackbox read timeout");
  }
  if(retry>4)stop();
  else{
    timer.start();
    aRead->setEnabled(false);
    aErase->setEnabled(false);
    aRefresh->setEnabled(false);
  }
}
//=============================================================================
void BlackboxDownload::response_received(unsigned char cmd, const QByteArray data)
{
  //qDebug()<<"BlackboxDownload::resp"<<cmd<<data.size();
  if(data.size()<3)return;
  if(cmd!=bb_read)return;
  switch((uint8_t)data.at(0)){
    default: return;
    case bbr_hdr: {
      if(data.size()!=(1+sizeof(_bb_hdr)))return;
      memcpy(&hdr,data.data()+1,sizeof(_bb_hdr));
      lbSize->setText(QString("%1 MB").arg((double)hdr.rec_size*(512.0/1024.0/1024.0),0,'f',2));
      stage=stage_idle;
    } break;
    case bbr_data: {
      if(data.size()!=(1+4+512))return;
      const uint8_t *p=(const uint8_t *)data.data()+1;
      uint32_t b=*p++;
      b|=*p++<<8;
      b|=*p++<<16;
      b|=*p++<<24;
      //qDebug()<<"bbr_data"<<b;
      if(req_blk!=b) return;
      //block ok
      reader.push(data.mid(5));
      retry=0;
      req_blk++;
      progressBar->setValue(req_blk);
      if(req_blk>=hdr.rec_size){
        qDebug("%s",tr("Download finished").toUtf8().data());
        stage=stage_idle;
        break;
      }

    } break;
  }
  next();
}
//=============================================================================
void BlackboxDownload::stop()
{
  if(stage==stage_idle)return;
  if(!do_stop) qWarning("%s",tr("Download aborted").toUtf8().data());
  do_stop=true;
  next();
}
//=============================================================================
void BlackboxDownload::on_aRead_triggered()
{
  stop();
  progressBar->setValue(0);
  progressBar->setMaximum(hdr.rec_size);
  progressBar->setVisible(true);
  req_blk=0;
  stage=stage_read;
  qDebug("%s",tr("Download started").toUtf8().data());
  mvar->rec->uavNameOverride=eUAV->text();
  mvar->rec->setRecording(true);
  emit started();
  next();
}
//=============================================================================
void BlackboxDownload::on_aAbort_triggered()
{
  stop();
}
//=============================================================================
void BlackboxDownload::on_aErase_triggered()
{
  stop();
  stage=stage_erase;
  emit started();
  next();
}
//=============================================================================
void BlackboxDownload::on_aRefresh_triggered()
{
  stop();
  stage=stage_init;
  emit started();
  next();
}
//=============================================================================
//=============================================================================
