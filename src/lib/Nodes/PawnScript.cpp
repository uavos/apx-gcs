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
#include "PawnScript.h"
#include "NodeItem.h"
#include "Nodes.h"
#include "PawnCompiler.h"
#include <crc.h>
#include <AppDirs.h>
#include <Vehicle.h>
//=============================================================================
PawnScript::PawnScript(NodeField *fact)
 : QObject(fact),fact(fact)
{
  op=op_idle;

  pawncc=new PawnCompiler(fact);
  connect(fact,&NodeField::valueChanged,this,&PawnScript::compile);
}
//=============================================================================
int PawnScript::size() const
{
  return flash_data_wr.size();
}
//=============================================================================
bool PawnScript::error() const
{
  return pawncc->error();
}
bool PawnScript::isBusy(void) const
{
  return op!=op_idle;
}
//=============================================================================
//=============================================================================
void PawnScript::compile()
{
  pawncc->compile();

  //pack flash_data file
  _ft_script hdr;
  memset(&hdr,0,sizeof(hdr));
  QByteArray basrc=qCompress(fact->value().toString().toUtf8(),9);
  QByteArray code=pawncc->outData();
  hdr.size=code.size()+basrc.size();
  hdr.code_size=code.size();
  code.append(basrc);
  hdr.crc=CRC_16_IBM((const uint8_t*)code.data(),code.size(),0xFFFF);
  //QString s=scrName;
  //if(s.size()>=(int)sizeof(hdr.name))s.resize(sizeof(hdr.name)-1);
  //memcpy(hdr.name,s.toUtf8().data(),s.size()+1);
  flash_data_wr.clear();
  flash_data_wr.append((const char*)&hdr,sizeof(hdr));
  flash_data_wr.append(code);
  //qDebug()<<"Script recompiled: "<<flash_data_wr.size()<<fact->ba_conf_bkp.size();
  //qDebug()<<hdr.size<<hdr.code_size<<hdr.crc;
  emit changed();
}
//=============================================================================
void PawnScript::unpackFlashData()
{
  //check consistency
  const QByteArray &ba=flash_data_rd;
  fact->setValue(QString());
  QString err;
  bool bErr=true;
  while(1){
    err="";//"small";
    if(ba.size()<=(int)sizeof(_ft_script))
      break;
    _ft_script hdr;
    memcpy(&hdr,ba.data(),sizeof(_ft_script));
    //qDebug()<<hdr.size<<hdr.code_size<<hdr.crc;
    err="size";
    if((uint)ba.size()!=(sizeof(_ft_script)+hdr.size)){
      //qDebug()<<ba.size()<<(sizeof(_ft_script)+hdr.size);
      break;
    }
    err="code size";
    if(hdr.code_size>=hdr.size)
      break;
    err="crc";
    if(hdr.crc!=CRC_16_IBM((const uint8_t*)ba.data()+sizeof(_ft_script),hdr.size,0xFFFF))
      break;
    QByteArray basrc=ba.mid(sizeof(_ft_script)+hdr.code_size);
    if(basrc.isEmpty())fact->setValue(QString());
    else{
      QString s=QString(qUncompress(basrc));
      if(s.isEmpty()) s=basrc;
      fact->setValue(s);
    }
    //qDebug("Script from '%s' downloaded",fact->node->title().toUtf8().data());
    /*QByteArray bacode=ba.mid(sizeof(_ft_script),hdr.code_size);
    QString s=bacode.toHex().toUpper();
    while(s.size()){
      qDebug()<<s.left(16*2);
      s.remove(0,16*2);
    }*/
    err="";
    bErr=false;
    break;
  }
  if(bErr){
    if(!err.isEmpty())
      qWarning("%s '%s' %s (%s)",tr("Script").toUtf8().data(),fact->node->title().toUtf8().data(),tr("error").toUtf8().data(),err.toUtf8().data());
    fact->setValue(QString());
  }
  //compile();

  fact->setDataValid(true);
}
//=============================================================================
//=============================================================================
void PawnScript::unpackService(unsigned char cmd, const QByteArray &data)
{
  if(cmd!=apc_script_file && data.size()==0)return;
  switch(cmd){
    case apc_script_file: {
      //qDebug("apc_script_file (cnt:%u)",data.size());
      if(op==op_read_hdr){
        if(data.size()!=sizeof(_flash_file)){
          //qWarning("Wrong flash_rfile reply size from '%s' (exp: %u re: %u)",fact->node->model->mvar->node_name(fact->node->sn),(uint)sizeof(_flash_file),data.size());
          return;
        }
        memcpy(&flash_rfile,data.data(),data.size());
        if(flash_rfile.size==0){
          fact->setDataValid(true);
          break;
        }
        flash_data_rd.resize(flash_rfile.size);
        //start reading flash blocks
        memset(&flash_block_hdr,0,sizeof(flash_block_hdr));
        op=op_read_data;
        //qDebug("op_read_data (%u)",flash_rfile.size);
        if(request_download())return;
        //qWarning("Script from '%s' is empty",fact->node->model->mvar->node_name(fact->node->sn));
        fact->setDataValid(true);
        break;
      }else if(op==op_write_hdr){
        if(data.size()==0){
          //write file acknowledged
          //qDebug("apc_script_file (cnt:%u)",data.size());
          qDebug("%s...",tr("Script upload started").toUtf8().data());
          op=op_write_data;
          flash_block_hdr.start_address=0;
          flash_block_hdr.data_size=0;
          request_upload();
          return;
        }
      }
    }return;
    case apc_script_read: {
      if(op!=op_read_data)return;
      //qDebug("apc_script_read (cnt:%u)",data.size());
      int sz=flash_block_hdr.data_size+sizeof(flash_block_hdr);
      if(data.size()!=sz){
        //qWarning("Wrong script_block reply size from '%s' (exp: %u re: %u)",fact->node->model->mvar->node_name(fact->node->sn),sz,data.size());
        //return;
      }else if(memcmp(&flash_block_hdr,data.data(),sizeof(flash_block_hdr))!=0){
        //qWarning("Wrong script_block reply sequence from '%s'",fact->node->model->mvar->node_name(fact->node->sn));
        //return;
      }else if((flash_block_hdr.start_address+flash_block_hdr.data_size)>(uint)flash_data_rd.size()){
        //qWarning("Wrong script_block reply from '%s'",fact->node->model->mvar->node_name(fact->node->sn));
        //return;
      }else{
        //qDebug("apc_script_read (%.4X, %u)",flash_block_hdr.start_address,data.size());
        memcpy(flash_data_rd.data()+flash_block_hdr.start_address,data.data()+sizeof(flash_block_hdr),flash_block_hdr.data_size);
        flash_block_hdr.start_address+=flash_block_hdr.data_size;
      }
      if(request_download())return;
      //qDebug("apc_script_read done");
      //downloaded, check consistency
      uint8_t xor_crc=0;
      for(int i=0;i<flash_data_rd.size();i++)
        xor_crc^=((uint8_t*)flash_data_rd.data())[i];
      if(xor_crc!=flash_rfile.xor_crc){
        qWarning("Wrong script_block CRC from '%s' (exp: %.2X re: %.2X)",fact->node->title().toUtf8().data(),xor_crc,flash_rfile.xor_crc);
        flash_data_rd.clear();
      }
      unpackFlashData();
      flash_data_rd.clear();
    }break;
    case apc_script_write: {
      if(op!=op_write_data)return;
      //if(isValid())break;
      //qDebug("apc_script_write (cnt:%u)",data.size());
      if(data.size()!=sizeof(flash_block_hdr)){
        //qWarning("Wrong script_write reply size from '%s' (exp: %u re: %u)",fact->node->model->mvar->node_name(fact->node->sn),(uint)sizeof(flash_block_hdr),data.size());
        //return;
      }else if(memcmp(&flash_block_hdr,data.data(),sizeof(flash_block_hdr))!=0){
        //qWarning("Wrong script_write reply sequence from '%s'",fact->node->model->mvar->node_name(fact->node->sn));
        //return;
      }else flash_block_hdr.start_address+=flash_block_hdr.data_size;
      if(request_upload())return;
      //uploaded
      qDebug("%s",tr("Script uploaded").toUtf8().data());
      fact->backup();
    }break;
    default:
      return;
  }
  //if(op)qDebug("Script op done (%s) %u",fact->node->title().toUtf8().data(),op);
  op=op_idle;
}
//=============================================================================
bool PawnScript::request_download(void)
{
  if(op!=op_read_data)return true;
  if(flash_block_hdr.start_address>=flash_rfile.size)
    return false;
  uint cnt=sizeof(_flash_data::data);
  uint rcnt=flash_rfile.size-flash_block_hdr.start_address;
  flash_block_hdr.data_size=cnt<rcnt?cnt:rcnt;
  fact->node->request(apc_script_read,QByteArray((const char*)&flash_block_hdr,sizeof(flash_block_hdr)),500);
  return true;
}
//=============================================================================
bool PawnScript::request_upload(void)
{
  if(op!=op_write_data)return true;
  if(flash_block_hdr.start_address>=flash_wfile.size)
    return false;
  uint cnt=sizeof(_flash_data::data);
  uint rcnt=flash_wfile.size-flash_block_hdr.start_address;
  flash_block_hdr.data_size=cnt<rcnt?cnt:rcnt;
  QByteArray ba((const char*)&flash_block_hdr,sizeof(flash_block_hdr));
  ba.append(QByteArray(flash_data_wr.data()+flash_block_hdr.start_address,flash_block_hdr.data_size));
  fact->node->request(apc_script_write,ba,1000);
  //qDebug()<<"upl: "<<flash_block_hdr.start_address<<flash_block_hdr.data_size<<ba.size();
  return true;
}
//=============================================================================
void PawnScript::download(void)
{
  op=op_read_hdr;
  fact->node->request(apc_script_file,QByteArray(),500);
}
//=============================================================================
void PawnScript::upload(void)
{
  if(error()){
    qWarning("%s",tr("Script error").toUtf8().data());
    return;
  }
  if(flash_data_wr.isEmpty())return;
  if(op!=op_idle){
    qWarning("%s",tr("Script is downloading").toUtf8().data());
    return;
  }
  //qDebug("Script upload");
  flash_wfile.start_address=0;
  flash_wfile.size=flash_data_wr.size();
  flash_wfile.xor_crc=0;
  for(int i=0;i<flash_data_wr.size();i++)
    flash_wfile.xor_crc^=((uint8_t*)flash_data_wr.data())[i];
  QByteArray ba((const char*)&flash_wfile,sizeof(_flash_file));
  op=op_write_hdr;
  fact->node->request(apc_script_file,ba,500);
}
//=============================================================================
void PawnScript::modelDone(void)
{
  op=op_idle;
}
//=============================================================================
