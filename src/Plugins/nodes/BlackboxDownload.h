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
#ifndef BlackboxDownload_H
#define BlackboxDownload_H
#include "ui_BlackboxDownload.h"
#include "NodesItemNode.h"
#include "QMandalaItem.h"
#include "node.h"
#include "EscReader.h"
//=============================================================================
class BlackboxDownload: public QWidget, public Ui::BlackboxDownload
{
  Q_OBJECT
public:
  explicit BlackboxDownload(NodesItemNode *node, QWidget *parent = 0);
  ~BlackboxDownload();
protected:
  void closeEvent(QCloseEvent *event);
private:
  Ui::BlackboxDownload *ui;
  NodesItemNode *node;
  QMandalaItem *mvar;

  QTimer timer;
  uint retry;
  bool do_stop;

  typedef enum {
    stage_idle=0,
    stage_init,
    stage_read,
    stage_erase,
  }_stage;
  _stage stage;
  uint bb_read;
  uint bb_erase;

  _bb_hdr hdr;
  uint32_t req_blk;

  EscReader reader;

private slots:
  void next();

  void on_aRead_triggered();
  void on_aAbort_triggered();
  void on_aErase_triggered();
  void on_aRefresh_triggered();

signals:
  void started();
  void finished();

public:
  //data exchange
  void response_received(unsigned char cmd, const QByteArray data);
signals:
  void request(uint cmd,const QByteArray sn,const QByteArray data,uint timeout_ms);
public slots:
  void stop();

};
//=============================================================================
#endif //BlackboxDownload_H
