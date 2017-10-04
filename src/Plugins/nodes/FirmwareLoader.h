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
#ifndef FirmwareLoader_H
#define FirmwareLoader_H
#include <QtCore>
#include "node.h"
class NodesItemNode;
//=============================================================================
class FirmwareLoader: public QObject
{
  Q_OBJECT
public:
  FirmwareLoader(NodesItemNode *node);

  bool busy;
  bool do_stop,do_loader;
  uint stage;

  uint progress() const;
  void setProgress(uint v);

  QString status() const;

  void response_received(unsigned char cmd, QByteArray data);
private:
  NodesItemNode *node;
  uint progressCnt,progressCnt_s;
  QTimer timer;
  uint retry;
  static const QString prefix;

  QString statusString;
  void log(QString s);

  void start();

  bool loadFile(QString filePrefix=QString());
  QByteArray loadHexFile(QString fileName);

  _flash_file ldr_mem,ldr_file;
  QByteArray fileData;

  _flash_data wp;
  uint wcnt,tcnt; //stats

  uint ncmd;

  void ldr_req(unsigned char cmd,const void *data,uint cnt);
  void ldr_req(unsigned char cmd,const QByteArray data=QByteArray());

  bool loadFileMHX(QString ver);

private slots:
  void next();

signals:
  void request(uint cmd,const QByteArray sn,const QByteArray data,uint timeout_ms);
  void done();
  void started();
public slots:
  void upgradeFirmware();
  void upgradeLoader();
  void upgradeMHX();
  void stop();
};
//=============================================================================
#endif
