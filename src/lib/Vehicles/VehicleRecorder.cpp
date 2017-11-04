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
#include "VehicleRecorder.h"
#include <QProgressBar>
#include <QProgressDialog>
#include <QMessageBox>
#include <QApplication>
#include <math.h>
#include <cmath>
#include <cfloat>
#include "AppDirs.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
#include "Nodes.h"

#include "node.h"
#include "Mandala.h"
//=============================================================================
VehicleRecorder::VehicleRecorder(Vehicle *parent)
  :Fact(parent,"recorder",tr("Recorder"),tr("Telemetry file recorder"),FactItem,NoData),
  vehicle(parent),
  v_mode(vehicle),
  v_stage(vehicle),
  v_dl_timestamp(vehicle),
  m_recording(false),
  m_fileSize(0)
{
  if(!AppDirs::telemetry().exists()) AppDirs::telemetry().mkpath(".");

  recFileSuffix=".datalink";
  recDisable=false;
  header_written=false;
  recTrigger=false;
  monitorTimer.setInterval(1000);
  connect(&monitorTimer,SIGNAL(timeout()),this,SLOT(monitorTimerTimeout()));

  recStopTimer.setSingleShot(true);
  connect(&recStopTimer,SIGNAL(timeout()),this,SLOT(stop_recording()));

  recFlightNo=recFileNames().size()+1;
  setRecording(QSettings().value("recording",false).toBool());
  //QTimer::singleShot(1000,this,SLOT(convertFormat()));

  //status change (size/time)
  connect(this,&VehicleRecorder::fileSizeChanged,this,&VehicleRecorder::updateStatus);
  updateStatus();
}
//=============================================================================
void VehicleRecorder::updateStatus()
{
  uint vsz=fileSize();
  QString sz;
  if(vsz<(1024*1024)) sz=QString("%1KB").arg(vsz/1024.0,0,'f',0);
  else if(vsz<(1024*1024*1024)) sz=QString("%1MB").arg(vsz/(1024.0*1024.0),0,'f',1);
  else sz=QString("%1GB").arg(vsz/(1024.0*1024.0*1024.0),0,'f',2);

  QDateTime t=fileTime();
  QDateTime t0=QDateTime::fromSecsSinceEpoch(0);
  QString st;
  if(t==t0)st="--:--";
  else if(t.secsTo(t0)<(60*60))st=t.toString("mm:ss");
  else if(t.daysTo(t0)<=0)st=t.toString("hh:mm:ss");
  else st=QString("%1d%2").arg(t.daysTo(t0)).arg(t.toString("hh:mm"));

  setStatus(QString("%1/%2").arg(sz).arg(st));
}
//=============================================================================
//=============================================================================
void VehicleRecorder::record_data(QString tag,const QByteArray &data)
{
  if(!record_check())return;
  const _bus_packet &packet=*(_bus_packet*)data.data();
  uint data_cnt=data.size();
  if(data_cnt<bus_packet_size_hdr)return;
  data_cnt-=bus_packet_size_hdr;
  QString sv;
  QHash<QString,QString> attrs;
  //QMandalaField *f=mvar->field(packet.id);
  //if(packet.id==idx_data)qDebug()<<tag<<data.toHex();
  switch(packet.id){
    case idx_downstream: {
      if(streamList.size()){
        int i=0;
        foreach(VehicleMandalaFact *f,vehicle->f_mandala->allFacts()){
          double v=f->unpackedValue();
          if(streamList.at(i)==v)sv+=',';
          else{
            streamList[i]=v;
            sv+=valueToString(v,f->precision())+",";
          }
          i++;
        }
      }else{
        foreach(VehicleMandalaFact *f,vehicle->f_mandala->allFacts()){
          double v=f->unpackedValue();
          streamList.append(v);
          sv+=valueToString(v,f->precision())+",";
        }
      }
      sv.remove(sv.size()-1,1);
      xmlWriter.writeTextElement("S",sv);
    }return;
    case idx_service: {
      if(data_cnt<(bus_packet_size_hdr_srv-bus_packet_size_hdr))return;
      QByteArray sn((const char*)packet.srv.sn,sizeof(_node_sn));
      data_cnt-=bus_packet_size_hdr_srv-bus_packet_size_hdr;
      QByteArray srv_data((const char*)packet.srv.data,data_cnt);
      if(packet.srv.cmd==apc_msg){
        tag="msg";
        attrs.insert("sn",sn.toHex().toUpper());
        NodeItem *node=vehicle->f_nodes->node(sn);
        if(node)attrs.insert("node_name",node->title());
        sv=QString(srv_data);
        break;
      }
      if(is_apc_conf(data))return;
      sv=srv_data.toHex();
      attrs.insert("f","idx_service");
      attrs.insert("sn",sn.toHex().toUpper());
      attrs.insert("node_name",vehicle->f_nodes->node(sn)->title());
      attrs.insert("cmd",QString::number(packet.srv.cmd));
    } break;
    case idx_set: {
      VehicleMandalaFact *f=vehicle->f_mandala->factById(packet.data[0]|packet.data[1]<<8);
      if(!f)return;
      attrs.insert("f",f->name());
      sv=valueToString(f->unpackedValue(),f->precision());
    } break;

    default: {
      VehicleMandalaFact *f=vehicle->f_mandala->factById(packet.data[0]|packet.data[1]<<8);
      if(packet.id>=idxPAD && f && f->id() && (!f->name().startsWith("gcu_"))){
        //regular var update
        //extract with local Mandala as global status is unknown
        attrs.insert("f",f->name());
        sv=valueToString(f->unpackedValue(),f->precision());
      }else{
        //unknown var or binary data
        QString vname=vehicle->f_mandala->special.key(packet.id,QString::number(packet.id));
        sv=QByteArray((const char*)packet.data,data_cnt).toHex().toUpper();
        attrs.insert("f",vname);
      }
    }break;
  }
  xmlWriter.writeStartElement(tag);
  foreach(const QString &attr,attrs.keys()){
    const QString &s=attrs.value(attr);
    if(s.size())xmlWriter.writeAttribute(attr,s);
  }
  xmlWriter.writeCharacters(sv);
  xmlWriter.writeEndElement();
}
void VehicleRecorder::record_downlink(const QByteArray &data)
{
  if(data.size()<=1)return;
  record_data("D",data);
}
void VehicleRecorder::record_uplink(const QByteArray &data)
{
  const _bus_packet &packet=*(_bus_packet*)data.data();
  if(packet.id==idx_downstream)return;
  record_data("U",data);
}
void VehicleRecorder::record_xml(const QByteArray &data)
{
  if(recording()&&header_written){
    //xmlWriter.device()->write("\n");
    xmlWriter.device()->write(data);
  }
}
//=============================================================================
QString VehicleRecorder::valueToString(double v, uint prec)
{
  if(!prec) return QString::number((uint)v);
  return QString::number(v,'f',prec).remove(QRegExp("0+$")).remove(QRegExp("\\.$"));
}
//=============================================================================
bool VehicleRecorder::record_check(void)
{
  if(recDisable){
    setRecording(false);
    return false;
  }
  if(vehicle->f_streamType->value().toInt()==Vehicle::TELEMETRY) {
    if((v_mode==mode_TAKEOFF)&&(v_stage>=2)&&(v_stage<100)){
      if(!recTrigger){
        close();
        setRecording(true);
        recTrigger=true;
      }
    }else recTrigger=false;
    if (recording()) {
      if((!recStopTimer.isActive()) && (v_mode==mode_LANDING)&&(v_stage>=250))
        recStopTimer.start(2000);
    }
  }
  if (!recording()){
    return false;
  }

  if (recFile.handle()<0) {
    //create new file...
    QStringList list(recFileNames());
    list.sort();
    recFlightNo=list.size()+1;
    //create next filename..
    QString fname,sname=QDateTime::currentDateTime().toString("yyyy'_'MM'_'dd'_'hh'_'mm'_'");
    uint i=list.size()?list.at(list.size()-1).mid(11,3).toInt()+1:1;

    QString title=vehicle->title()+uavNameSuffix;
    if(title.size())QSettings().setValue("recordingTitle",title);
    else title=QSettings().value("recordingTitle").toString();
    do {
      fname=sname+QString().sprintf("%.3u_",i++)+title+recFileSuffix;
    } while (list.contains(fname));
    recFile.setFileName(AppDirs::telemetry().filePath(fname));
    recFile.open(QIODevice::WriteOnly);
    xmlWriter.setDevice(&recFile);
    record_header();
  }
  return true;
}
//=============================================================================
void VehicleRecorder::stop_recording()
{
  setRecording(false);
}
//=============================================================================
void VehicleRecorder::record_header(void)
{
  xmlWriter.setAutoFormatting(true);
  xmlWriter.setAutoFormattingIndent(2);
  xmlWriter.writeStartDocument();
  xmlWriter.writeStartElement("telemetry.gcu.uavos.com");
  xmlWriter.writeAttribute("href","http://www.uavos.com/");
  xmlWriter.writeAttribute("title",QFileInfo(recFile.fileName()).baseName());
  xmlWriter.writeAttribute("UTC",QDateTime::currentDateTime().toUTC().toString(Qt::ISODate));
  xmlWriter.writeAttribute("time_ms",QString::number(v_dl_timestamp));

  xmlWriter.writeStartElement("mandala");
  xmlWriter.writeTextElement("version",FactSystem::version());
  xmlWriter.writeTextElement("hash",vehicle->f_mandala->md5().toHex().toUpper());
  xmlWriter.writeTextElement("fields",vehicle->f_mandala->names.join(","));
  xmlWriter.writeEndElement();

  foreach(const QByteArray &t,saveXmlParts.values())
    if(t.size())xmlWriter.device()->write(t);
  xmlWriter.setAutoFormattingIndent(0);
  header_written=true;
  recStartTime=QDateTime::currentDateTime();
}
//=============================================================================
void VehicleRecorder::saveXmlPart(QString key,QByteArray data)
{
  saveXmlParts.insert(key,data);
  record_xml(data);
}
//=============================================================================
void VehicleRecorder::flush()
{
  if (recFile.handle()>=0) {
    recFile.flush();
  }
}
//=============================================================================
const QStringList VehicleRecorder::recFileNames(void)
{
  return QDir(AppDirs::telemetry().absolutePath(),"*"+recFileSuffix).entryList(QDir::Files,QDir::Name);
}
//=============================================================================
bool VehicleRecorder::loadFlight(int idx,QProgressBar *progressBar)
{
  const QStringList &filesList=recFileNames();
  if ((idx<0)||(idx>=filesList.size()))return false;
  return loadFlight(AppDirs::telemetry().filePath(filesList.at(idx)),progressBar);
}
//=============================================================================
bool VehicleRecorder::loadFlight(const QString &fName,QProgressBar *progressBar)
{
  QTime t;
  t.start();
  loadFile.setFileName(fName);
  loadFile.open(QIODevice::ReadOnly|QIODevice::Text);
  if(loadFile.size()>64*1024*1024){
    int rv=QMessageBox::question(
      NULL,QApplication::applicationName(),
      tr("Telemetry file is too big (%1 MB). Load it?").arg(loadFile.size()/(1024*1024)),QMessageBox::Yes|QMessageBox::No);
    if(rv==QMessageBox::No){
      loadFile.close();
      return false;
    }
  }
  if(progressBar){
    progressBar->setMaximum(loadFile.size());
    progressBar->reset();
  }
  file.time.clear();
  file.data.clear();
  file.msg.clear();
  file.params.clear();
  file.xmlParts.clear();
  file.notes.clear();

  //check file format
  QString fline(loadFile.readLine(255));
  loadFile.reset();
  if(fline.startsWith("#datalink.gcu.uavos.com: ")){
    loadFromText(progressBar);
  }else{
    loadFromXml(progressBar);
  }

  setFileSize(loadFile.size());
  loadFile.close();

  //parse date time
  file.timestamp=QDateTime::fromString(QFileInfo(loadFile).baseName().left(16),"yyyy_MM_dd_HH_mm");
  if(!file.timestamp.isValid())
    file.timestamp=QFileInfo(loadFile).created();

  emit fileLoaded();
  return true;
}
//=============================================================================
void VehicleRecorder::loadFromXml(QProgressBar *progressBar)
{
  const QString doc_tag="telemetry.gcu.uavos.com";
  QXmlStreamReader xml(&loadFile);
  if (xml.readNextStartElement()&&(xml.name()!=doc_tag)){
    xml.raiseError(tr("The file format is not correct."));
    return;
  }

  QList<int>map; //downlink stream map to idx in doublelist
  int dl_timestamp_idx=-1;
  uint time_ms=0,time_start=0;
  uint time_s=0;
  uint pass=0;
  while(xml.readNextStartElement()){
    if((++pass&0x0FFF)==0){
      QCoreApplication::flush();
      QCoreApplication::processEvents();
      QCoreApplication::flush();
    }
    const QString tag=xml.name().toString();
    if(tag=="mandala"){
      while(xml.readNextStartElement()){
        if(xml.name()=="fields"){
          const QStringList &st=xml.readElementText().split(',');
          map.clear();
          foreach(VehicleMandalaFact *f,vehicle->f_mandala->allFacts()){
            int i=st.indexOf(f->name());
            map.append(i);
            if(f->id()==idx_dl_timestamp)
              dl_timestamp_idx=i;
          }
        }else xml.skipCurrentElement();
      }
    }else if(tag=="D"||tag=="U"||tag=="S"){
      if(xml.attributes().isEmpty()){
        //values downstream
        const QStringList &st=xml.readElementText().split(',');
        if(dl_timestamp_idx>=0 && dl_timestamp_idx<st.size() && st.at(dl_timestamp_idx).size()){
          //find dl_timestamp
          uint t_ms=st.at(dl_timestamp_idx).toUInt();
          uint dt_ms=t_ms-time_s;
          time_s=t_ms;
          if(dt_ms<1)dt_ms=100;
          else if(dt_ms>1000)dt_ms=1000;
          time_ms+=dt_ms;
          if(!time_start){
            time_start=time_ms;
            time_ms-=time_start;
          }
          //qDebug()<<dt_ms<<time_ms<<time_start<<t_ms;
          file.time.append(time_ms);
          //append data
          ListDouble vlist;
          int fcnt=vehicle->f_mandala->allFacts().size();
          if(tag=="S"){
            for(int i=0;i<fcnt;i++){
              int mi=map.at(i);
              if(mi<0)vlist.append(0);
              else if(st.at(mi).size()){
                double v=st.at(mi).toDouble();
                if(std::isnan(v)||std::isinf(v))v=0;
                vlist.append(v);
              }else if(file.data.size())vlist.append(file.data.last().at(i));
              else vlist.append(0);
            }
          }else{
            for(int i=0;i<fcnt;i++){
              int mi=map.at(i);
              if(mi<0)vlist.append(0);
              else vlist.append(st.at(mi).size()?st.at(mi).toDouble():0);
            }
          }
          file.data.append(vlist);
        }
      }else if(xml.attributes().hasAttribute("f")){
        //var update downlink/uplink
        if(tag=="D"){
          xml.skipCurrentElement();
        }else{
          QString s=xml.attributes().value("f").toString()+": ";
          s+=xml.readElementText();
          file.msg[time_ms].append(s);
        }
      }
    }else if(tag=="msg"){
      QString s=tag+": ["+xml.attributes().value("node_name").toString()+"] ";
      s+=xml.readElementText();
      file.msg[time_ms].append(s);
    }else if(tag=="notes"){
      file.notes=xml.readElementText();
    }else{
      //read and save generic xmlPart
      file.xmlParts[tag].insertMulti(time_ms,readXmlPart(xml));
    }
    if(progressBar) progressBar->setValue(loadFile.pos());
  }
  if(pass==0 || xml.tokenType()==QXmlStreamReader::EndElement)return;
  //fix unterminated end tag
  loadFile.close();
  loadFile.open(QIODevice::Append);
  loadFile.write(QString("</"+doc_tag+">\n").toUtf8().data());
  loadFile.close();
  loadFile.open(QIODevice::ReadOnly);
  qWarning("%s",tr("Telemetry file end tag fixed").toUtf8().data());
}
//=============================================================================
QString VehicleRecorder::readXmlPart(QXmlStreamReader &xml)
{
  const QString tag=xml.name().toString();
  QString xmlPart;
  QXmlStreamWriter writer(&xmlPart);
  writer.writeCurrentToken(xml); //start tag
  while(!xml.atEnd()) {
    xml.readNext();
    if(xml.isEndElement() && xml.name()==tag)break;
    writer.writeCurrentToken(xml);
  }
  writer.writeCurrentToken(xml); //end tag
  return xmlPart;
}
//=============================================================================
void VehicleRecorder::loadFromText(QProgressBar *progressBar)
{
  qWarning("%s",tr("Old telemetry file format detected").toUtf8().data());
  char buf[65535];
  QList<int>map; //downlink binary stream map to idx in doublelist
  uint pass=0;
  bool err=false,hdr=true;
  uint time_ms=0,time_shift=0;
  int dlsz=0; //check downstream size match
  while (!err) {
    if((++pass&0x0FFF)==0){
      QCoreApplication::flush();
      QCoreApplication::processEvents();
      QCoreApplication::flush();
    }
    int cnt=loadFile.readLine(buf,sizeof(buf));
    if (cnt<=0)break;
    switch(buf[0]){
      case '<':{  //downlink compressed qbytearray
        if(cnt<5){err=true;break;}
        const QByteArray &ba=qUncompress(QByteArray::fromHex(QByteArray(buf+1,cnt-1)));
        if(ba.size()!=dlsz){err=true;break;}
        const int fcnt=vehicle->f_mandala->allFacts().size();
        if(map.size()!=fcnt){err=true;break;}
        const uint8_t *p=(const uint8_t*)ba.data();
        uint32_t t=*((uint32_t*)p);
        p+=sizeof(uint32_t);
        t+=time_shift;
        if(t<time_ms){
          time_shift=time_ms+1000;
          t+=time_shift;
        }
        time_ms=t;
        file.time.append(time_ms);
        const float *vp=(const float*)p;
        ListDouble vlist;
        for(int i=0;i<fcnt;i++)
          vlist.append(map.at(i)<0?0:std::isfinite(vp[map.at(i)])?vp[map.at(i)]:0);
        file.data.append(vlist);
      }break;
      case '>':{  //some uplink text
        const QString s(QByteArray((const char*)buf,cnt));
        //check nodes conf
        if(s.contains(":")){
          const QString sname=s.left(s.indexOf(':')).trimmed();
          if(sname==">service"){
            const QByteArray &ba=QByteArray::fromHex(s.right(s.length()-s.indexOf(':')-1).trimmed().toUtf8());
            if(ba.size()<(int)bus_packet_size_hdr_srv)break;
            uint cmd=ba.at(bus_packet_size_hdr_srv-1);
            if(cmd!=apc_conf_write)break;
          }
        }
        file.msg[time_ms].append(QByteArray((const char*)buf,cnt));
      }break;
      case '-': hdr=false; break;
      default:{
        if(!hdr){
          const QString s(QByteArray((const char*)buf,cnt));
          //check nodes conf
          if(s.contains(":")){
            const QString sname=s.left(s.indexOf(':')).trimmed();
            /*if(sname=="service"){
              const QByteArray &ba=QByteArray::fromHex(s.right(s.length()-s.indexOf(':')-1).trimmed().toUtf8());
              if(is_apc_conf(ba))
                load_apc_conf(ba);
              break;
            }*/
          }
          //some downlink text
          file.msg[time_ms].append(s);
          break;
        }
        //header
        const QString &sline=QByteArray(buf,cnt);
        if(!sline.contains(":"))break;
        const QString &sname=sline.left(sline.indexOf(':')).trimmed();
        const QString &svalue=sline.right(sline.length()-sline.indexOf(':')-1).trimmed();
        file.params[sname]=svalue;
        if(sname=="Vars"){
           const QStringList &st=svalue.split(',');
           dlsz=sizeof(uint32_t)+st.size()*sizeof(float);
           map.clear();
           foreach(VehicleMandalaFact *f,vehicle->f_mandala->allFacts())
             map.append(st.indexOf(f->name()));
        }
      }//default
    }//switch
    if(progressBar) progressBar->setValue(loadFile.pos());
  }//while lines
}
//=============================================================================
double VehicleRecorder::fileValue(const QString &name, int pos)
{
  if(file.data.isEmpty() || pos>=file.data.size())return 0;
  int fi=vehicle->f_mandala->names.indexOf(name);
  if(fi<0)return 0;
  return file.data.at(pos).at(fi);
}
//=============================================================================
bool VehicleRecorder::is_apc_conf(const QByteArray &ba)
{
  if(ba.size()<(int)bus_packet_size_hdr_srv)return false;
  const _bus_packet &packet=*(_bus_packet*)ba.data();
  if(packet.id!=idx_service)return false;
  uint cmd=packet.srv.cmd;
  if(cmd==apc_info || cmd==apc_conf_inf || cmd==apc_conf_dsc || cmd==apc_conf_cmds || cmd==apc_conf_read || cmd==apc_conf_write)return true;
  return false;
}
//=============================================================================
bool VehicleRecorder::recording() const
{
  return m_recording;
}
void VehicleRecorder::setRecording(bool v)
{
  if(m_recording==v)return;
  m_recording=v;
  QSettings().setValue("recording",v);
  emit recordingChanged();
  if(v){
    monitorTimer.start();
  }else{
    monitorTimer.stop();
    close();
    recTrigger=false;
  }
}
void VehicleRecorder::close(void)
{
  header_written=false;
  streamList.clear();
  if (recFile.handle()>=0) {
    xmlWriter.writeEndElement(); //telemetry
    xmlWriter.writeEndDocument();
    flush();
    recFile.close();
  }
}
void VehicleRecorder::discard(void)
{
  close();
  recFile.remove();
}
//=============================================================================
uint VehicleRecorder::fileSize() const
{
  return m_fileSize;
}
void VehicleRecorder::setFileSize(uint v)
{
  if(m_fileSize==v)return;
  m_fileSize=v;
  emit fileSizeChanged();
}
void VehicleRecorder::monitorTimerTimeout(void)
{
  if(recFile.handle()>=0) {
    setFileSize(recFile.size());
    //split to 1Gb files
    if(recFile.size()>(1024*1024*1024))close();
  }
}
QDateTime VehicleRecorder::fileTime() const
{
  if(recFile.handle()>=0) {
    return QDateTime::fromSecsSinceEpoch(0).addSecs(recStartTime.secsTo(QDateTime::currentDateTime()));
  }
  return QDateTime::fromSecsSinceEpoch(0).addSecs(file.time.size()?file.time.last()/1000:0);
}
//=============================================================================
