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
#include "QMandala.h"
#include "PawnScript.h"
#include "NodesItemNode.h"
#include "NodesModel.h"
#include "crc.h"
#include "AppDirs.h"
//=============================================================================
PawnScript::PawnScript(NodesItemField *field)
 : QObject(field),field(field)
{
  connect(this,SIGNAL(request(uint,QByteArray,QByteArray,uint)),field,SIGNAL(request(uint,QByteArray,QByteArray,uint)));
  connect(this,SIGNAL(changed()),field,SIGNAL(changed()));

  connect(field->node->model,SIGNAL(done()),this,SLOT(modelDone()));

  data_valid=false;
  op=op_idle;

  //const uint vm_data_size=1024;
  tmpFile.open();
  outFileName=tmpFile.fileName()+"-compiled.amx";
  QStringList args;
  //qDebug()<<path_inc;
  args<<"-d0";
  args<<"-O3";
  args<<"-v2";
  args<<"-r";
  //args<<"-l";
  //args<<"-a";
  //fill mandala constants
  foreach(QMandalaField *f,field->node->model->mvar->fields)
    args<<"f_"+f->name()+"="+QString::number(f->varmsk());
  foreach(QString name,field->node->model->mvar->constants.keys())
    args<<name+"="+QString::number(field->node->model->mvar->constants.value(name));

  //args<<"-S"+QString::number((vm_data_size-256)/4);
  //args<<"-XD"+QString::number(vm_data_size);
  args<<"-i"+AppDirs::res().absoluteFilePath("scripts/pawn/include");
  args<<"-i"+AppDirs::scripts().absoluteFilePath("pawn");
  args<<"-i"+AppDirs::scripts().absoluteFilePath(".");
  args<<"-o"+outFileName;
  args<<tmpFile.fileName();
  pawncc.setProgram(QCoreApplication::applicationDirPath()+"/pawncc");
  pawncc.setArguments(args);
  pawncc.setProcessChannelMode(QProcess::MergedChannels);
  //qDebug()<<args;
}
//=============================================================================
QString PawnScript::getSource() const
{
  return QString(script_source);
}
//=============================================================================
int PawnScript::size() const
{
  return flash_data.size();
}
//=============================================================================
bool PawnScript::isError() const
{
  return script_source.trimmed().size()&&flash_data.isEmpty();
}
bool PawnScript::isValid(void) const
{
  return data_valid;
}
bool PawnScript::isEmpty(void) const
{
  return size()<=(int)sizeof(_flash_file);
}
bool PawnScript::isModified(void) const
{
  if(flash_data.size()!=flash_data_bkp.size())return true;
  return memcmp(flash_data.data(),flash_data_bkp.data(),flash_data.size())!=0;
}
void PawnScript::invalidate(void)
{
  data_valid=false;
  emit changed();
}
void PawnScript::validate(void)
{
  data_valid=true;
  backup();
}
void PawnScript::backup(void)
{
  //qDebug("Script backup");
  flash_data_bkp=flash_data;
  emit changed();
}
void PawnScript::restore(void)
{
  setSourceDataFile(flash_data_bkp);
}
const QByteArray & PawnScript::data() const
{
  return flash_data;
}
//=============================================================================
//=============================================================================
bool PawnScript::setSource(QByteArray ba)
{
  flash_data.clear();
  script_source=ba;
  bool bEmpty=ba.trimmed().isEmpty();
  if(bEmpty)return true;
  QByteArray code;
  if(!compile()){
    emit changed();
    return false;
  }
  //pack flash_data file
  QFile file(outFileName);
  if (!file.open(QFile::ReadOnly)) {
    //qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(outFileName).arg(file.errorString()).toUtf8().data());
    emit changed();
    return false;
  }
  code=file.readAll();
  _ft_script hdr;
  memset(&hdr,0,sizeof(hdr));
  QByteArray basrc=qCompress(script_source,9);
  hdr.size=code.size()+basrc.size();
  hdr.code_size=code.size();
  code.append(basrc);
  hdr.crc=CRC_16_IBM((const uint8_t*)code.data(),code.size(),0xFFFF);
  //QString s=scrName;
  //if(s.size()>=(int)sizeof(hdr.name))s.resize(sizeof(hdr.name)-1);
  //memcpy(hdr.name,s.toUtf8().data(),s.size()+1);
  flash_data.append((const char*)&hdr,sizeof(hdr));
  flash_data.append(code);
  //qDebug()<<"Script recompiled: "<<flash_data.size()<<field->ba_conf_bkp.size();
  //qDebug()<<hdr.size<<hdr.code_size<<hdr.crc;
  emit changed();
  return true;
}
//=============================================================================
bool PawnScript::setSourceDataFile(QByteArray ba)
{
  //check consistency
  flash_data=ba;
  script_source.clear();
  QString err;
  bool bErr=true;
  while(1){
    err="";//"small";
    if(flash_data.size()<=(int)sizeof(_ft_script))
      break;
    _ft_script hdr;
    memcpy(&hdr,flash_data.data(),sizeof(_ft_script));
    //qDebug()<<hdr.size<<hdr.code_size<<hdr.crc;
    err="size";
    if((uint)flash_data.size()!=(sizeof(_ft_script)+hdr.size)){
      //qDebug()<<flash_data.size()<<(sizeof(_ft_script)+hdr.size);
      break;
    }
    err="code size";
    if(hdr.code_size>=hdr.size)
      break;
    err="crc";
    if(hdr.crc!=CRC_16_IBM((const uint8_t*)flash_data.data()+sizeof(_ft_script),hdr.size,0xFFFF))
      break;
    QByteArray basrc=flash_data.mid(sizeof(_ft_script)+hdr.code_size);
    if(basrc.isEmpty())script_source.clear();
    else{
      script_source=qUncompress(basrc);
      if(script_source.isEmpty())
        script_source=basrc;
    }

    //qDebug("Script from '%s' downloaded",field->node->model->mvar->node_name(field->node->sn));
    /*QByteArray bacode=flash_data.mid(sizeof(_ft_script),hdr.code_size);
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
      qWarning("%s '%s' %s (%s)",tr("Script").toUtf8().data(),field->node->model->mvar->node_name(field->node->sn),tr("error").toUtf8().data(),err.toUtf8().data());
    flash_data.clear();
    script_source.clear();
    emit changed();
    return false;
  }
  return setSource(script_source);
}
//=============================================================================
bool PawnScript::compile()
{
  tmpFile.resize(0);
  tmpFile.flush();
  QFile::remove(outFileName);
  QString src=getSource();
  if(src.trimmed().isEmpty())return true;
  //qDebug()<<src;
  QTextStream s(&tmpFile);
  s << src;
  s.flush();
  tmpFile.flush();
  pawncc.start();
  bool rv=true;
  if(!pawncc.waitForFinished()){
    qDebug() << "error:" << pawncc.errorString();
    rv=false;
  }else if(pawncc.exitCode()!=0)rv=false;
  pawncc_log=pawncc.isOpen()?pawncc.readAll():QString();
  if(!rv)pawncc_log.append("\n\n"+pawncc.errorString());

  //log file
  QTemporaryFile logFile;
  logFile.setFileTemplate(QDir::tempPath()+"/pawncc_log");
  logFile.setAutoRemove(false);
  if(logFile.open()){
    QTextStream s(&logFile);
    s << pawncc.program() + " " + pawncc.arguments().join(' ');
    s << "\n\n";
    s << pawncc_log;
    s.flush();
    logFile.flush();
    logFile.close();
  }
  //qDebug()<<"compile";
  return rv;
}
//=============================================================================
QString PawnScript::getLog()
{
  return pawncc.isOpen()?pawncc_log:QString();
}
//=============================================================================
//=============================================================================
void PawnScript::response_received(unsigned char cmd,const QByteArray data)
{
  if(cmd!=apc_script_file && data.size()==0)return;
  switch(cmd){
    case apc_conf_read: {
      if(op!=op_idle)return;
      if(data_valid)return;
      //qDebug("ft_script apc_conf_read (sz: %u)",((_ft_script*)data.data())->size);
      flash_data.clear();
      script_source.clear();
      op=op_read_hdr;
      emit request(apc_script_file,field->node->sn,QByteArray(),500);
    }return;
    case apc_script_file: {
      //qDebug("apc_script_file (cnt:%u)",data.size());
      if(op==op_read_hdr){
        if(data.size()!=sizeof(_flash_file)){
          //qWarning("Wrong flash_rfile reply size from '%s' (exp: %u re: %u)",field->node->model->mvar->node_name(field->node->sn),(uint)sizeof(_flash_file),data.size());
          return;
        }
        memcpy(&flash_rfile,data.data(),data.size());
        if(flash_rfile.size==0){
          validate();
          break;
        }
        flash_data.resize(flash_rfile.size);
        //start reading flash blocks
        memset(&flash_block_hdr,0,sizeof(flash_block_hdr));
        op=op_read_data;
        //qDebug("op_read_data (%u)",flash_rfile.size);
        if(request_download())return;
        //qWarning("Script from '%s' is empty",field->node->model->mvar->node_name(field->node->sn));
        validate();
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
        //qWarning("Wrong script_block reply size from '%s' (exp: %u re: %u)",field->node->model->mvar->node_name(field->node->sn),sz,data.size());
        //return;
      }else if(memcmp(&flash_block_hdr,data.data(),sizeof(flash_block_hdr))!=0){
        //qWarning("Wrong script_block reply sequence from '%s'",field->node->model->mvar->node_name(field->node->sn));
        //return;
      }else if((flash_block_hdr.start_address+flash_block_hdr.data_size)>(uint)flash_data.size()){
        //qWarning("Wrong script_block reply from '%s'",field->node->model->mvar->node_name(field->node->sn));
        //return;
      }else{
        //qDebug("apc_script_read (%.4X, %u)",flash_block_hdr.start_address,data.size());
        memcpy(flash_data.data()+flash_block_hdr.start_address,data.data()+sizeof(flash_block_hdr),flash_block_hdr.data_size);
        flash_block_hdr.start_address+=flash_block_hdr.data_size;
      }
      if(request_download())return;
      //qDebug("apc_script_read done");
      //downloaded, check consistency
      uint8_t xor_crc=0;
      for(int i=0;i<flash_data.size();i++)
        xor_crc^=((uint8_t*)flash_data.data())[i];
      if(xor_crc!=flash_rfile.xor_crc){
        qWarning("Wrong script_block CRC from '%s' (exp: %.2X re: %.2X)",field->node->model->mvar->node_name(field->node->sn),xor_crc,flash_rfile.xor_crc);
        flash_data.clear();
      }
      validate();
      setSourceDataFile(flash_data);
    }break;
    case apc_script_write: {
      if(op!=op_write_data)return;
      //if(isValid())break;
      //qDebug("apc_script_write (cnt:%u)",data.size());
      if(data.size()!=sizeof(flash_block_hdr)){
        //qWarning("Wrong script_write reply size from '%s' (exp: %u re: %u)",field->node->model->mvar->node_name(field->node->sn),(uint)sizeof(flash_block_hdr),data.size());
        //return;
      }else if(memcmp(&flash_block_hdr,data.data(),sizeof(flash_block_hdr))!=0){
        //qWarning("Wrong script_write reply sequence from '%s'",field->node->model->mvar->node_name(field->node->sn));
        //return;
      }else flash_block_hdr.start_address+=flash_block_hdr.data_size;
      if(request_upload())return;
      //uploaded
      qDebug("%s",tr("Script uploaded").toUtf8().data());
      backup();
    }break;
    default:
      return;
  }
  //if(op)qDebug("Script op done (%s) %u, %u",field->node->name.toUtf8().data(),op,data_valid);
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
  emit request(apc_script_read,field->node->sn,QByteArray((const char*)&flash_block_hdr,sizeof(flash_block_hdr)),500);
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
  ba.append(QByteArray(flash_data.data()+flash_block_hdr.start_address,flash_block_hdr.data_size));
  emit request(apc_script_write,field->node->sn,ba,1000);
  //qDebug()<<"upl: "<<flash_block_hdr.start_address<<flash_block_hdr.data_size<<ba.size();
  return true;
}
//=============================================================================
void PawnScript::upload(void)
{
  if(isError()){
    qWarning("%s",tr("Script error").toUtf8().data());
    return;
  }
  if(isEmpty())return;
  if(op!=op_idle){
    qWarning("%s",tr("Script is downloading").toUtf8().data());
    return;
  }
  //qDebug("Script upload");
  flash_wfile.start_address=0;
  flash_wfile.size=flash_data.size();
  flash_wfile.xor_crc=0;
  for(int i=0;i<flash_data.size();i++)
    flash_wfile.xor_crc^=((uint8_t*)flash_data.data())[i];
  QByteArray ba((const char*)&flash_wfile,sizeof(_flash_file));
  op=op_write_hdr;
  emit request(apc_script_file,field->node->sn,ba,500);
}
//=============================================================================
void PawnScript::modelDone(void)
{
  op=op_idle;
}
//=============================================================================
