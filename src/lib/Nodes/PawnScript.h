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
#ifndef PawnScript_H
#define PawnScript_H
#include "NodeField.h"
#include <node.h>
class PawnCompiler;
//=============================================================================
class PawnScript: public QObject
{
  Q_OBJECT
public:
  PawnScript(NodeField *field);

  void unpackFlashData();

  int size() const;
  bool error() const;

  //field status forward
  bool isBusy(void) const;

  void download(void);
  void upload(void);

  //data comm
  void unpackService(unsigned char cmd, const QByteArray &data);

private:
  NodeField *field;
  //ft_script field storage data
  enum{op_idle,op_read_hdr,op_write_hdr,op_read_data,op_write_data}op;

  //flash read/write
  QByteArray flash_data_rd,flash_data_wr;   //all file data
  _flash_file flash_rfile;
  _flash_file flash_wfile;
  _flash_data_hdr flash_block_hdr;

  bool request_download(void); //true if not done
  bool request_upload(void); //true if not done

  //compiler
  PawnCompiler *pawncc;

private slots:
  void compile();
  void modelDone(void);
signals:
  //void request(uint cmd,const QByteArray sn,const QByteArray data,uint timeout_ms);
  void changed(void);
};
//=============================================================================
#endif
