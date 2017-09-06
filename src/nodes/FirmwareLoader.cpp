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
#include "FirmwareLoader.h"
#include "NodesItemNode.h"
#include "NodesModel.h"
const QString FirmwareLoader::prefix="/usr/share/uavos/firmware";
//=============================================================================
FirmwareLoader::FirmwareLoader(NodesItemNode *node)
 : QObject(node),node(node)
{
  busy=do_stop=false;
  stage=0;

  connect(this,SIGNAL(done()),node->model,SLOT(firmwareUpgraded()));
  connect(this,SIGNAL(started()),&node->model->requestManager,SLOT(stop()));

  timer.setSingleShot(true);
  timer.setInterval(100);
  connect(&timer,SIGNAL(timeout()),this,SLOT(next()));

  connect(this,SIGNAL(request(uint,QByteArray,QByteArray,uint)),node,SIGNAL(request(uint,QByteArray,QByteArray,uint)));
}
//=============================================================================
void FirmwareLoader::upgradeFirmware()
{
  if(busy)return;
  qDebug("%s",QString("%1: %2 (%3)").arg(tr("Firmware upgrade")).arg(node->name).arg(node->data(NodesItem::tc_value).toString()).toUtf8().data());
  if(!loadFile()){
    emit done();
    return;
  }
  stage=0;
  start();
}
void FirmwareLoader::upgradeLoader()
{
  if(busy)return;
  qDebug("%s",QString("%1: %2 (%3)").arg(tr("Loader upgrade")).arg(node->name).arg(node->data(NodesItem::tc_value).toString()).toUtf8().data());
  if(!loadFile("loader-")){
    emit done();
    return;
  }
  stage=5;
  start();
}
void FirmwareLoader::upgradeMHX()
{
  if(busy)return;
  qDebug("%s",QString("%1: %2 (%3)").arg(tr("MHX radio upgrade")).arg(node->name).arg(node->data(NodesItem::tc_value).toString()).toUtf8().data());
  stage=100;
  start();
}
//=============================================================================
void FirmwareLoader::start()
{
  busy=true;
  do_stop=false;
  retry=0;
  progressCnt=0;
  log(tr("Initializing")+"...");
  emit started();
  next();
}
//=============================================================================
//=============================================================================
void FirmwareLoader::next()
{
  if(!busy)return;
  timer.stop();
  if(do_stop)stage=-1;
  switch(stage){
    //USER UPGRADE
    case 0:
      ncmd=apc_loader;
      do_loader=false;
      emit request(apc_loader,node->sn,QByteArray(),0);
      stage++;
      timer.start(100);
      return;
    case 1: //check ldc_init response
      if(retry>50){
        qWarning("%s",tr("Can't initialize loader").toUtf8().data());
        break;
      }
      ldr_req(ldc_init,QByteArray());
      retry++;
      timer.start(200);
      return;
      //LOADER UPGRADE
    case 5:
      ncmd=apc_loader;
      do_loader=true;
      if(retry>5){
        qWarning("%s",tr("Can't initialize upgrade").toUtf8().data());
        break;
      }
      ldr_req(ldc_init,QByteArray());
      retry++;
      timer.start(200);
      return;

    case 10: //after ldc_init (in loader)
      if(ldr_mem.start_address!=ldr_file.start_address){
        qWarning("%s 0x%.8X (avail: 0x%.8X).\n",tr("Invalid start address").toUtf8().data(),ldr_file.start_address,ldr_mem.start_address);
        break;
      }
      if(ldr_mem.size<ldr_file.size){
        qWarning("%s (%d).\n",tr("File too long").toUtf8().data(),ldr_mem.size-ldr_file.size);
        //break;
      }
      stage++;
    case 11:
      if(retry>3){
        qWarning("%s",tr("Can't setup loader file").toUtf8().data());
        break;
      }
      //write file info
      ldr_req(ldc_file,&ldr_file,sizeof(_flash_file));
      retry++;
      timer.start(500);
      return;
    case 12: //write data
      if(retry>(do_loader?5:3)){
        qWarning("%s",tr("No response from device").toUtf8().data());
        if(do_loader){
          stage=5;
          retry=0;
          next();
          return;
        }
        break;
      }
      if(wp.hdr.data_size>tcnt) wp.hdr.data_size=tcnt;
      memset(wp.data,0xFF,sizeof(wp.data));
      memcpy(wp.data,fileData.data()+wcnt,wp.hdr.data_size);
      ldr_req(ldc_write,&wp,sizeof(wp));
      timer.start(5000);
      if(retry)log(QString("%1... %2").arg(tr("Retrying")).arg(retry));
      retry++;
      return;

      //MHX firmware
    case 100: //get version info
      ncmd=apc_user;
      ldr_req(ldc_init);
      stage++;
      timer.start(500);
      return;
    case 101:
      if(retry>3){
        qWarning("%s",tr("Can't initialize MHX update").toUtf8().data());
        break;
      }
      stage=100;
      next();
      return;
    case 102: //init file write
      if(retry>0){
        qWarning("%s",tr("Can't initialize MHX file write").toUtf8().data());
        break;
      }
      ldr_req(ldc_file);
      retry++;
      timer.start(1000);
      return;
    case 103: //write data
      if(retry>8){
        qWarning("%s",tr("No response from device").toUtf8().data());
        break;
      }
      wp.data[0]=wp.hdr.start_address;
      wp.data[1]=wp.hdr.start_address>>8;
      memcpy(wp.data+2,((uint8_t*)fileData.data())+wcnt,wp.hdr.data_size);
      ldr_req(ldc_write,wp.data,wp.hdr.data_size+2);
      timer.start(1000);
      if(retry)log(QString("%1... %2").arg(tr("Retrying")).arg(retry));
      retry++;
      return;
    case 104: //start flash upgrade sequence
      if(retry>120){
        qWarning("%s",tr("No response from device").toUtf8().data());
        break;
      }
      if(!retry)ldr_req(ldc_write);
      log(QString("%1... %2").arg(tr("WAIT").toUpper()).arg(retry));
      timer.start(1000);
      retry++;
      return;


    default:
      break;
  }
  setProgress(0);
  busy=false;
  emit done();
}
//=============================================================================
void FirmwareLoader::response_received(unsigned char cmd, QByteArray data)
{
  //qDebug()<<"resp"<<cmd<<data;
  if(busy==false || cmd!=ncmd || data.isEmpty())return;
  cmd=((uint8_t*)data.data())[0];
  data.remove(0,1);
  switch(ncmd+cmd){
    case apc_loader+ldc_init:
      if(data.size()!=sizeof(_flash_file)){
        qWarning("Wrong ldc_init response (%u)",data.size());
        return;
      }
      memcpy(&ldr_mem,data.data(),sizeof(_flash_file));
      //qDebug("%s (%ukB, 0x%.8X).\n",node->name.toUtf8().data(),ldr_mem.size/1024,ldr_mem.start_address);
      stage=10;
      ldr_req(ldc_file,&ldr_file,sizeof(_flash_file));
    break;
    case apc_loader+ldc_file: //file confirm
      if(data.size()!=sizeof(_flash_file)){
        qWarning("Wrong ldc_file response (%u)",data.size());
        return;
      }
      if(memcmp(&ldr_file,data.data(),sizeof(_flash_file))!=0){
        qWarning("Wrong ldc_file check");
        return;
      }
      log(tr("Writing")+"...");
      //init write data
      if(!do_loader)node->clear();
      wp.hdr.start_address=ldr_file.start_address;
      wp.hdr.data_size=sizeof(wp.data);
      wcnt=0;
      tcnt=ldr_file.size;
      stage=12;
      timer.start(1);
    break;
    case apc_loader+ldc_write: //write wp confirm
      if(data.size()!=sizeof(wp.hdr)){
        qWarning("Wrong ldc_write response (%u)",data.size());
        return;
      }
      if(memcmp(&(wp.hdr),data.data(),sizeof(wp.hdr))!=0){
        qWarning("Wrong ldc_write check");
        return;
      }
      wp.hdr.start_address+=wp.hdr.data_size;
      tcnt-=wp.hdr.data_size;
      //stats
      wcnt+=wp.hdr.data_size;
      setProgress(100*wcnt/ldr_file.size);
      log(QString().sprintf("%.1fkB/%.1fkB...",wcnt/1024.0,ldr_file.size/1024.0));
      progressCnt_s=progressCnt;
      if(!tcnt)stage=-1;
      retry=0;
      next();
      return;

      //MHX
    case apc_user+ldc_init: //MHX version received
      if(data.size()<2){
        qWarning("Wrong ldc_init response (%u)",data.size());
        return;
      }
      if(loadFileMHX(QString(data))){
        stage=102;
      }else{
        qDebug("%s",tr("Update not needed").toUtf8().data());
        stage=-1;
      }
      timer.start(1);
      break;
    case apc_user+ldc_file: //MHX file accepted
      if(data.size()){
        qWarning("Wrong ldc_file response (%u)",data.size());
        return;
      }
      log(tr("Writing")+"...");
      //init write data
      wp.hdr.start_address=0;
      wp.hdr.data_size=128;
      wcnt=0;
      tcnt=ldr_file.size;
      stage=103;
      timer.start(1);
      break;
    case apc_user+ldc_write: //write wp confirm
      if(data.size()==0 && tcnt==0){
        log(tr("Success"));
        stage=-1;
        timer.start(1);
        break;
      }
      if(data.size()!=2){
        qWarning("Wrong ldc_write response (%u)",data.size());
        return;
      }
      if(memcmp(&(wp.data),data.data(),2)!=0){
        qWarning("Wrong ldc_write check");
        return;
      }
      wp.hdr.start_address++; //blk
      if(tcnt>wp.hdr.data_size)tcnt-=wp.hdr.data_size;
      else tcnt=0;
      //stats
      wcnt+=wp.hdr.data_size;
      setProgress(100*wcnt/ldr_file.size);
      log(QString().sprintf("%.1fkB/%.1fkB...",wcnt/1024.0,ldr_file.size/1024.0));
      progressCnt_s=progressCnt;
      if(!tcnt)stage=104;
      retry=0;
      next();
      return;
    default:
      return;
  }
  //cmd accepted and processed
  retry=0;
}
//=============================================================================
void FirmwareLoader::ldr_req(unsigned char cmd,const QByteArray data)
{
  ldr_req(cmd,data.data(),data.size());
}
void FirmwareLoader::ldr_req(unsigned char cmd,const void *data,uint cnt)
{
  QByteArray ba;
  ba.append((unsigned char)cmd);
  if(cnt)ba.append(QByteArray((const char*)data,cnt));
  emit request(ncmd,node->sn,ba,0);
}
//=============================================================================
//=============================================================================
void FirmwareLoader::stop()
{
  if(!busy)return;
  if(!do_stop) qWarning("%s",tr("Firmware upgrade aborted").toUtf8().data());
  do_stop=true;
  next();
}
//=============================================================================
uint FirmwareLoader::progress() const
{
  return busy?progressCnt:0;
}
//=============================================================================
void FirmwareLoader::setProgress(uint v)
{
  if(v==0)progressCnt_s=0;
  progressCnt=v;
  node->model->dataChanged(node->model_index,node->model_index);
}
//=============================================================================
QString FirmwareLoader::status() const
{
  return busy?statusString:"";
}
//=============================================================================
void FirmwareLoader::log(QString s)
{
  //qDebug()<<s;
  statusString=s;
  node->model->dataChanged(node->model_index,node->model_index);
}
//=============================================================================
//=============================================================================
bool FirmwareLoader::loadFile(QString filePrefix)
{
  QString hw((const char*)node->node_info.hardware);
  QString arch="armv7-m";
  QString name=(const char*)node->node_info.name;
  QString fname=filePrefix+name+".hex";
  QString fileName=QString("%1/%2/%3/%4").arg(prefix).arg(arch).arg(hw).arg(fname);
  //qDebug()<<fileName;
  fileData=loadHexFile(fileName);
  if(fileData.isEmpty())return false;
  return true;
}
//=============================================================================
QByteArray FirmwareLoader::loadHexFile(QString fileName)
{
  log(tr("Loading file")+"...");
  QFile file(fileName);
  if(!file.open(QFile::ReadOnly|QFile::Text)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fileName).arg(file.errorString()).toUtf8().data());
    return QByteArray();
  }
  QTextStream stream(&file);
  QByteArray ba;
  uint max_size=4*1024*1024; //PM_SIZE];  //PM data from file
  ba.resize(max_size);
  uint8_t *file_data=(uint8_t*)ba.data();
  //read file
  bool data_addr_init=false;
  uint lcnt=0;
  uint  ExtAddr = 0;
  uint data_cnt=0;  //number of data bytes in array
  uint data_addr=0; //start address of data in array
  uint cnt=0;
  while(!stream.atEnd()){
    QString line=stream.readLine();
    const char*bufLine=line.toUtf8().data();
    int ByteCount=0,Address=0,RecordType,v;
    sscanf(bufLine+1, "%2x%4x%2x", &ByteCount, &Address, &RecordType);
    lcnt++;
    switch (RecordType) {
    case 0: //data record
      Address += ExtAddr;
      if(!data_addr_init) {
        data_addr_init=true;
        data_addr=Address;
      }
      if(Address<(int)data_addr) {
        qWarning("%s (0x%.8X, line %u)\n",tr("Data address too low").toUtf8().data(),Address,lcnt);
      } else {
        for (int CharCount = 0; CharCount < ByteCount*2; CharCount += 2, Address++) {
          sscanf(bufLine+9+CharCount,"%2x",&v);
          if (Address>=(int)(data_addr+max_size)) {
            qWarning("%s (0x%.8X, line %u)\n",tr("Data address too high").toUtf8().data(),Address,lcnt);
            break;
          }
          uint i=Address-data_addr;
          file_data[i]=v;
          cnt++;
          if(data_cnt<(i+1))data_cnt=i+1;
          // get device type from file
          //if ((dtype>0xff) && (Address==dtype_addr))dtype=v;
        }
      }
      break;
    case 1: //End Of File
      break;
    case 5: //Start Linear Address Record. EIP register of the 80386 and higher CPU.
      break;
    case 4: //Extended Linear Address Record
      sscanf(bufLine+9, "%4x", &ExtAddr);
      ExtAddr = ExtAddr << 16;
      break;
    case 2: //Extended Linear Address Record
      sscanf(bufLine+9, "%4x", &ExtAddr);
      ExtAddr = ExtAddr << 4;
      break;
    default:
      qWarning("%s: %2X (line %u)\n",tr("Unknown hex record type").toUtf8().data(),RecordType,lcnt);
    }
  }
  ba.resize(cnt);
  ldr_file.start_address=data_addr;
  ldr_file.size=cnt;
  ldr_file.xor_crc=0;
  for(uint i=0;i<cnt;i++)ldr_file.xor_crc^=file_data[i];
  //qDebug()<<"File: "<<ldr_file.size;
  return ba;
}
//=============================================================================
bool FirmwareLoader::loadFileMHX(QString ver)
{
  bool bErr=true;
  //parse version
  QString mname;
  uint iver;
  while(1){
    if(!ver.contains('_'))break;
    mname=ver.left(ver.indexOf('_'));
    QString s=ver.mid(ver.indexOf('_')+1);
    if(s.startsWith('v',Qt::CaseInsensitive))s.remove(0,1);
    iver=s.toFloat()*1000;
    if(iver<1000 || mname.size()<4)break;
    qDebug("MHX radio: %s (ver %u)",mname.toUpper().toUtf8().data(),iver);
    bErr=false;
    break;
  }
  if(bErr){
    qWarning("%s (%s)",tr("Error parsing MHX version").toUtf8().data(),ver.toUtf8().data());
    return false;
  }
  //load corresponding file
  QString fileName;
  while(1){
    QDir dir(prefix);
    if(!dir.cd("mhx"))break;
    foreach(QFileInfo fi,dir.entryInfoList()){
      QString s=fi.baseName();
      if(!s.startsWith(mname,Qt::CaseInsensitive))continue;
      s=s.mid(ver.indexOf('_')+1);
      if(s.startsWith('v',Qt::CaseInsensitive))s.remove(0,1);
      uint iv=s.toUInt();
      if(iv<1000)continue;
      if(iv<=iver){
        bErr=false;
        qDebug("%s: %s",tr("Older firmware file").toUtf8().data(),fi.baseName().toUtf8().data());
        continue;
      }
      qDebug("%s: %s",tr("New firmware").toUtf8().data(),fi.baseName().toUtf8().data());
      fileName=fi.absoluteFilePath();
      bErr=false;
      break;
    }
    break;
  }
  if(bErr){
    qWarning("%s",tr("MHX firmware files not found").toUtf8().data());
    return false;
  }
  if(fileName.isEmpty())return false;
  log(tr("Loading file")+"...");
  QFile file(fileName);
  if(!file.open(QFile::ReadOnly)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fileName).arg(file.errorString()).toUtf8().data());
    return false;
  }

  fileData=file.readAll();
  ldr_file.start_address=0;
  ldr_file.size=fileData.size();
  //qDebug()<<fileData.size();
  return true;
}
//=============================================================================
//=============================================================================
