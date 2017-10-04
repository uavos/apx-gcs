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
#include "FlightDataFile.h"
#include "QMandala.h"
#include "QMandala.h"
#include <QProgressBar>
#include <QProgressDialog>
#include <QMessageBox>
#include <QApplication>
#include <math.h>
#include <cmath>
#include <cfloat>
//using namespace std;
//=============================================================================
FlightDataFile::FlightDataFile(QMandalaItem *parent)
  :QObject(parent), mandala(QMandala::instance()),mvar((QMandalaItem*)parent)
{
  if(!QMandala::Global::telemetry().exists()) QMandala::Global::telemetry().mkpath(".");

  recFileSuffix=".datalink";
  m_recording=false;
  recDisable=false;
  header_written=false;
  recTrigger=false;
  m_size=0;
  monitorTimer.setInterval(1000);
  connect(&monitorTimer,SIGNAL(timeout()),this,SLOT(monitorTimerTimeout()));

  recStopTimer.setSingleShot(true);
  connect(&recStopTimer,SIGNAL(timeout()),this,SLOT(stop_recording()));

  recFlightNo=recFileNames().size()+1;
  setRecording(QSettings().value("recording",false).toBool());
  //QTimer::singleShot(1000,this,SLOT(convertFormat()));
}
//=============================================================================
void FlightDataFile::record_data(QString tag,const QByteArray &data)
{
  if(!record_check())return;
  const _bus_packet &packet=*(_bus_packet*)data.data();
  uint data_cnt=data.size();
  if(data_cnt<bus_packet_size_hdr)return;
  data_cnt-=bus_packet_size_hdr;
  QString sv;
  QHash<QString,QString> attrs;
  QMandalaField *f=mvar->field(packet.id);
  //if(packet.id==idx_data)qDebug()<<tag<<data.toHex();
  switch(packet.id){
    case idx_downstream: {
      if(streamList.size()){
        int i=0;
        foreach(QMandalaField *f,mvar->fields){
          double v=f->value();
          if(streamList.at(i)==v)sv+=',';
          else{
            streamList[i]=v;
            sv+=fieldToString(f,v)+",";
          }
          i++;
        }
      }else{
        foreach(QMandalaField *f,mvar->fields){
          double v=f->value();
          streamList.append(v);
          sv+=fieldToString(f,v)+",";
        }
      }
      sv.remove(sv.size()-1,1);
      xmlWriter.writeTextElement("S",sv);
      /*foreach(QMandalaField *f,mvar->fields){
        double v=f->value();
        sv+=v==0?",":(QString::number(v,'g',8)+",");
      }
      sv.remove(sv.size()-1,1);
      xmlWriter.writeTextElement(tag,sv);*/
    }return;
  case idx_service: {
    if(data_cnt<(bus_packet_size_hdr_srv-bus_packet_size_hdr))return;
    QByteArray sn((const char*)packet.srv.sn,sizeof(_node_sn));
    data_cnt-=bus_packet_size_hdr_srv-bus_packet_size_hdr;
    QByteArray srv_data((const char*)packet.srv.data,data_cnt);
    if(packet.srv.cmd==apc_msg){
      tag="msg";
      attrs.insert("sn",sn.toHex().toUpper());
      attrs.insert("node_name",mvar->node_name(sn));
      sv=QString(srv_data);
      break;
    }
    if(is_apc_conf(data))return;
    sv=srv_data.toHex();
    attrs.insert("f",mvar->var_name(packet.id));
    attrs.insert("sn",sn.toHex().toUpper());
    attrs.insert("node_name",mvar->node_name(sn));
    attrs.insert("cmd",QString::number(packet.srv.cmd));
  } break;
  case idx_set: {
    f=mvar->field(packet.data[0]|packet.data[1]<<8);
    attrs.insert("f",f->name());
    _var_float v;
    mvar->unpack_float_f4(packet.data+2,&v);
    sv=QString::number(v);
  } break;

  default:
    if(packet.id>=idxPAD && f->varmsk() && (!f->name().startsWith("gcu_"))){
      //regular var update
      //extract with local Mandala as global status is unknown
      attrs.insert("f",f->name());
      m.extract(packet.data,data_cnt,packet.id);
      sv=fieldToString(f,m.get_data(packet.id));
    }else{
      //unknown var or binary data
      QString vname;
      if(f->varmsk())vname=f->name();
      else{
        uint type;
        void *value_ptr;
        if(!mvar->get_ptr(packet.id,&value_ptr,&type))return;
        const char *var_name;
        const char *var_descr;
        if(!mvar->get_text_names(packet.id|0xFF00,&var_name,&var_descr))return;
        vname=QString(var_name);
      }
      sv=QByteArray((const char*)packet.data,data_cnt).toHex();
      attrs.insert("f",vname);
    }
  }
  xmlWriter.writeStartElement(tag);
  foreach(const QString &attr,attrs.keys()){
    const QString &s=attrs.value(attr);
    if(s.size())xmlWriter.writeAttribute(attr,s);
  }
  xmlWriter.writeCharacters(sv);
  xmlWriter.writeEndElement();
}
void FlightDataFile::record_downlink(const QByteArray &data)
{
  if(data.size()<=1)return;
  record_data("D",data);
}
void FlightDataFile::record_uplink(const QByteArray &data)
{
  const _bus_packet &packet=*(_bus_packet*)data.data();
  if(packet.id==idx_downstream)return;
  record_data("U",data);
}
void FlightDataFile::record_xml(const QByteArray &data)
{
  if(recording()&&header_written){
    //xmlWriter.device()->write("\n");
    xmlWriter.device()->write(data);
  }
}
//=============================================================================
QString FlightDataFile::fieldToString(QMandalaField *f,double v)
{
  uint prec=f->precision();
  if(!prec) return QString::number((uint)v);
  return QString::number(v,'f',prec).remove(QRegExp("0+$")).remove(QRegExp("\\.$"));
}
//=============================================================================
bool FlightDataFile::record_check(void)
{
  if(recDisable){
    setRecording(false);
    return false;
  }
  if (mandala->online()) {
    if((mvar->mode==mode_TAKEOFF)&&(mvar->stage>=2)&&(mvar->stage<100)){
      if(!recTrigger){
        close();
        setRecording(true);
        recTrigger=true;
      }
    }else recTrigger=false;
    if (recording()) {
      if((!recStopTimer.isActive()) && (mvar->mode==mode_LANDING)&&(mvar->stage>=250))
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

    QString title;
    if(uavNameOverride.isEmpty()){
      title=mandala->uavName();
      if(mandala->current==mandala->local && mandala->size()<1){
        if(!mandala->current->apcfg.value("comment").toString().isEmpty()){
          title=mandala->current->apcfg.value("comment").toString();
          QSettings().setValue("recUavNameLocal",title);
        }else title=QSettings().value("recUavNameLocal",title).toString();
      }
    }else{
      title=uavNameOverride;
    }
    if(title.size())QSettings().setValue("recordingTitle",title);
    else title=QSettings().value("recordingTitle").toString();
    do {
      fname=sname+QString().sprintf("%.3u_",i++)+title+recFileSuffix;
    } while (list.contains(fname));
    recFile.setFileName(QMandala::Global::telemetry().filePath(fname));
    recFile.open(QIODevice::WriteOnly);
    xmlWriter.setDevice(&recFile);
    record_header();
  }
  return true;
}
//=============================================================================
void FlightDataFile::stop_recording()
{
  setRecording(false);
}
//=============================================================================
void FlightDataFile::record_header(void)
{
  xmlWriter.setAutoFormatting(true);
  xmlWriter.setAutoFormattingIndent(2);
  xmlWriter.writeStartDocument();
  xmlWriter.writeStartElement("telemetry.gcu.uavos.com");
  xmlWriter.writeAttribute("href","http://www.uavos.com/");
  xmlWriter.writeAttribute("title",QFileInfo(recFile.fileName()).baseName());
  xmlWriter.writeAttribute("UTC",QDateTime::currentDateTime().toUTC().toString(Qt::ISODate));
  xmlWriter.writeAttribute("time_ms",QString::number(mvar->dl_timestamp));

  xmlWriter.writeStartElement("mandala");
  xmlWriter.writeTextElement("version",QMandala::version);
  xmlWriter.writeTextElement("hash",mvar->md5.toHex());
  xmlWriter.writeTextElement("fields",QStringList(mvar->names).join(","));
  xmlWriter.writeEndElement();

  foreach(const QByteArray &t,saveXmlParts.values())
    if(t.size())xmlWriter.device()->write(t);
  xmlWriter.setAutoFormattingIndent(0);
  header_written=true;
  recStartTime=QDateTime::currentDateTime();
}
//=============================================================================
void FlightDataFile::saveXmlPart(QString key,QByteArray data)
{
  saveXmlParts.insert(key,data);
  record_xml(data);
}
//=============================================================================
void FlightDataFile::flush()
{
  if (recFile.handle()>=0) {
    recFile.flush();
  }
}
//=============================================================================
const QStringList FlightDataFile::recFileNames(void)
{
  return QDir(QMandala::Global::telemetry().absolutePath(),"*"+recFileSuffix).entryList(QDir::Files,QDir::Name);
}
//=============================================================================
bool FlightDataFile::loadFlight(int idx,QProgressBar *progressBar)
{
  const QStringList &filesList=recFileNames();
  if ((idx<0)||(idx>=filesList.size()))return false;
  return loadFlight(QMandala::Global::telemetry().filePath(filesList.at(idx)),progressBar);
}
//=============================================================================
bool FlightDataFile::loadFlight(const QString &fName,QProgressBar *progressBar)
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

  setSize(loadFile.size());
  loadFile.close();

  //parse date time
  file.timestamp=QDateTime::fromString(QFileInfo(loadFile).baseName().left(16),"yyyy_MM_dd_HH_mm");
  if(!file.timestamp.isValid())
    file.timestamp=QFileInfo(loadFile).created();

  emit fileLoaded();
  return true;
}
//=============================================================================
void FlightDataFile::loadFromXml(QProgressBar *progressBar)
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
          foreach(QMandalaField *f,mvar->fields){
            int i=st.indexOf(f->name());
            map.append(i);
            if(f->varmsk()==idx_dl_timestamp)
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
          if(tag=="S"){
            for(int i=0;i<mvar->fields.size();i++){
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
            for(int i=0;i<mvar->fields.size();i++){
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
QString FlightDataFile::readXmlPart(QXmlStreamReader &xml)
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
void FlightDataFile::loadFromText(QProgressBar *progressBar)
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
        if(map.size()!=mvar->fields.size()){err=true;break;}
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
        for(int i=0;i<mvar->fields.size();i++)
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
           foreach(QMandalaField *f,mvar->fields)
             map.append(st.indexOf(f->name()));
        }
      }//default
    }//switch
    if(progressBar) progressBar->setValue(loadFile.pos());
  }//while lines
}
//=============================================================================
double FlightDataFile::value(const QString &name, int pos)
{
  if(file.data.isEmpty() || pos>=file.data.size())return 0;
  int fi=mvar->names.indexOf(name);
  if(fi<0)return 0;
  return file.data.at(pos).at(fi);
}
//=============================================================================
bool FlightDataFile::is_apc_conf(const QByteArray &ba)
{
  if(ba.size()<(int)bus_packet_size_hdr_srv)return false;
  const _bus_packet &packet=*(_bus_packet*)ba.data();
  if(packet.id!=idx_service)return false;
  uint cmd=packet.srv.cmd;
  if(cmd==apc_info || cmd==apc_conf_inf || cmd==apc_conf_dsc || cmd==apc_conf_cmds || cmd==apc_conf_read || cmd==apc_conf_write)return true;
  return false;
}
//=============================================================================
bool FlightDataFile::recording()
{
  return m_recording;
}
void FlightDataFile::setRecording(bool v)
{
  if(m_recording==v)return;
  m_recording=v;
  QSettings().setValue("recording",v);
  emit recordingChanged(m_recording);
  if(v){
    monitorTimer.start();
  }else{
    monitorTimer.stop();
    close();
    recTrigger=false;
  }
}
void FlightDataFile::close(void)
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
void FlightDataFile::discard(void)
{
  close();
  recFile.remove();
}
//=============================================================================
uint FlightDataFile::size()
{
  return m_size;
}
void FlightDataFile::setSize(uint v)
{
  if(m_size==v)return;
  m_size=v;
  emit sizeChanged(m_size);
}
void FlightDataFile::monitorTimerTimeout(void)
{
  if(recFile.handle()>=0) {
    setSize(recFile.size());
    //split to 1Gb files
    if(recFile.size()>(1024*1024*1024))close();
  }
}
QTime FlightDataFile::time()
{
  if(recFile.handle()>=0) {
    return QTime(0,0).addSecs(recStartTime.secsTo(QDateTime::currentDateTime()));
  }
  return QTime(0,0).addSecs(file.time.size()?file.time.last()/1000:0);
}
//=============================================================================
