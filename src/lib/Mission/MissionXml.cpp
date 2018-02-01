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
#include "MissionXml.h"
#include <FactSystem.h>
#include <AppDirs.h>
//=============================================================================
MissionXml::MissionXml(QObject *parent)
  : QObject(parent),
    m_progress(0)
{
  //database
  _db = new TelemetryDB(this,QLatin1String("GCSMissionXmlSession"));
}
//=============================================================================
void MissionXml::setProgress(int v)
{
  if(m_progress==v)return;
  m_progress=v;
  emit progressChanged(v);
}
//=============================================================================
//=============================================================================
quint64 MissionXml::read(QString fileName)
{
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fileName).arg(file.errorString()).toUtf8().data());
    return 0;
  }
  const QString doc_tag="telemetry.gcu.uavos.com";
  QXmlStreamReader xml(&file);
  if (xml.readNextStartElement()&&(xml.name()!=doc_tag)){
    xml.raiseError(tr("The file format is not correct."));
    return 0;
  }

  QString ftitle=QFileInfo(fileName).baseName();
  quint64 telemetryID=0;

  if(!_db->isOpen())return 0;
  QSqlQuery query(*_db);
  bool ok=true;
  while(ok){

    //get timestamp from file
    quint64 timestamp=0;
    //try UTC attr
    if(xml.attributes().hasAttribute("UTC")){
      QDateTime ftime=QDateTime::fromString(xml.attributes().value("UTC").toString(),Qt::ISODate);
      timestamp=ftime.toMSecsSinceEpoch();
      //qDebug()<<xml.attributes().value("UTC").toString();
      //qDebug()<<ftime.toUTC().toString(Qt::ISODate);
      //qDebug()<<timestamp;
    }
    ok=timestamp!=0;
    if(!ok){
      qWarning("%s",QString("%1: %2").arg(tr("Can't get timestamp for file")).arg(fileName).toUtf8().data());
      break;
    }
    //find callsign from file
    QString callsign;
    QStringList st=ftitle.split('_');
    if(st.size()>=7){
      QStringList st2;
      for(int i=6;i<st.size();++i)st2.append(st.at(i));
      callsign=st2.join('_');
    }
    if(callsign.isEmpty()){
      callsign="UNKNOWN";
      qWarning("%s",QString("%1: %2").arg(tr("Callsign")).arg(callsign).toUtf8().data());
    }

    //check already imported data
    query.prepare("SELECT * FROM Telemetry WHERE callsign=? AND comment=? AND rec=1 AND timestamp=?");
    query.addBindValue(callsign);
    query.addBindValue(ftitle);
    query.addBindValue(timestamp);
    ok=query.exec();
    if(!ok)break;
    if(query.next()){
      qDebug("%s",QString("%1: %2").arg(tr("Already imported")).arg(ftitle).toUtf8().data());
      return query.value(0).toULongLong();
    }

    //register telemetry data file
    telemetryID=_db->writeRecord(QString(),callsign,ftitle,false,timestamp);
    ok=telemetryID;
    if(!ok)break;


    //collect available DB fields
    QHash<QString,quint64> recFields;
    foreach (Fact *f, _db->recFacts.keys()) {
      recFields.insert(f->name(),_db->recFacts.value(f));
    }

    //read XML file
    qDebug("%s",QString("%1...").arg(tr("Importing XML")).toUtf8().data());
    QList<quint64> recFieldsMap;
    QList<double> recValues;
    int dl_timestamp_idx=-1;
    uint time_ms=0,time_start=0;
    uint time_s=0;
    //uint pass=0;
    while(xml.readNextStartElement()){
      /*if((++pass&0x0FFF)==0){
        QCoreApplication::flush();
        QCoreApplication::processEvents();
        QCoreApplication::flush();
      }*/
      const QString tag=xml.name().toString();
      if(tag=="mandala"){
        //-----------------------------------------------------------
        while(xml.readNextStartElement()){
          if(xml.name()=="fields"){
            const QStringList &st=xml.readElementText().split(',');
            recFieldsMap.clear();
            for(int i=0;i<st.size();++i)recFieldsMap.append(0);
            for(int i=0;i<st.size();++i){
              QString s=st.at(i);
              if(!recFields.contains(s))continue;
              recFieldsMap[i]=recFields.value(s);
              if(s=="dl_timestamp") dl_timestamp_idx=i;
            }
          }else xml.skipCurrentElement();
        }
      }else if(tag=="D"||tag=="U"||tag=="S"){
        //-----------------------------------------------------------
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
            //append data
            _db->transaction(query);
            for(int i=0;i<recFieldsMap.size();++i){
              bool brec=false;
              double v=st.at(i).toDouble(&brec);
              if(!brec)v=0;
              if(i>=recValues.size()){
                recValues.append(v);
                brec=true;
              }else if(brec && recValues.at(i)==v)brec=false;
              if(!brec)continue;
              recValues[i]=v;
              //write value update
              ok=_db->writeField(query,telemetryID,time_ms,recFieldsMap.at(i),v,false);
              if(!ok)break;
            }
            _db->commit(query);
          }
        }else if(xml.attributes().hasAttribute("f")){
          //var update downlink/uplink
          const QString &fname=xml.attributes().value("f").toString();
          quint64 fieldID=recFields.value(fname);
          if(fname=="xpdr"){
            _db->writeEvent(telemetryID,time_ms,"xpdr","raw",false,QByteArray::fromHex(xml.readElementText().toUtf8()));
          }else if(fieldID==0 || fname.startsWith("gcu_"))xml.skipCurrentElement();
          else{
            double v=xml.readElementText().toDouble();
            int i=recFieldsMap.indexOf(fieldID);
            bool bUplink=tag=="U";
            if(bUplink==false && i>=0 && i<recValues.size()) recValues[i]=v;
            //write value update
            ok=_db->writeField(telemetryID,time_ms,fieldID,v,bUplink);
            if(!ok)break;
          }
        }
      }else if(tag=="msg"){
        //-----------------------------------------------------------
        const QByteArray &sn=QByteArray::fromHex(xml.attributes().value("sn").toString().toUtf8());
        QString s="["+xml.attributes().value("node_name").toString()+"]";
        s+=xml.readElementText();
        _db->writeEvent(telemetryID,time_ms,"msg",s,false,sn);
      }else if(tag=="notes"){
        //-----------------------------------------------------------
        ok=_db->writeNotes(telemetryID,xml.readElementText());
        if(!ok)break;
      }else if(tag=="node_conf"){
        //-----------------------------------------------------------
        const QByteArray &sn=QByteArray::fromHex(xml.attributes().value("sn").toString().toUtf8());
        const QString &stitle=xml.attributes().value("name").toString();
        QString fname=xml.attributes().value("f").toString();
        bool bArray=false;
        if(fname.contains('[')){
          fname=fname.left(fname.indexOf('['));
          bArray=true;
        }
        while(xml.readNextStartElement()){
          if(xml.name()=="value"){
            QString fxname=fname;
            if(xml.attributes().hasAttribute("name")){
              fxname.append(QString("_%1").arg(xml.attributes().value("name").toString()));
            }else if(xml.attributes().hasAttribute("idx")){
              fxname.append(QString("_%1").arg(xml.attributes().value("idx").toString()));
            }
            const QString &svalue=xml.readElementText();
            const QString &s=QString("%1/%2=%3").arg(stitle).arg(fxname).arg(svalue);
            _db->writeEvent(telemetryID,time_ms,"conf",s,true,sn);

          }else xml.skipCurrentElement();
        }
      }else if(tag=="nodes"){
        //-----------------------------------------------------------
        const QByteArray &ba=readXmlPart(xml);
        _db->writeEvent(telemetryID,time_ms,"nodes","xml",false,ba);
        QXmlStreamReader xmlp(ba);
        xmlp.readNextStartElement();
        //extract vehicle ident
        while(xmlp.readNextStartElement()){
          if(xmlp.name()=="ident"){
            QString callsign,vuid;
            while(xmlp.readNextStartElement()){
              if(xmlp.name()=="callsign")callsign=xmlp.readElementText();
              else if(xmlp.name()=="uid")vuid=xmlp.readElementText();
              else xmlp.skipCurrentElement();
            }
            if(!(callsign.isEmpty()||vuid.isEmpty())){
              //update vehicle info
              query.prepare("UPDATE Telemetry SET vehicleUID=?, callsign=? WHERE key = ?");
              query.addBindValue(vuid);
              query.addBindValue(callsign);
              query.addBindValue(telemetryID);
              query.exec();
            }
          }else xmlp.skipCurrentElement();
        }
      }else if(tag=="mission"){
        //-----------------------------------------------------------
        _db->writeEvent(telemetryID,time_ms,"mission","xml",true,readXmlPart(xml));
      }else{
        //-----------------------------------------------------------
        //unknown tag
        xml.skipCurrentElement();
      }
      //-----------------------------------------------------------
      query.finish();
      setProgress(file.pos()*100/file.size());
      QCoreApplication::processEvents();
      if(!ok)break;
    }

    break; //while ok
  }

  setProgress(0);

  if(telemetryID){
    //remove rec flag
    query.prepare("UPDATE Telemetry SET rec=? WHERE key = ?");
    query.addBindValue(ok?1:QVariant());
    query.addBindValue(telemetryID);
    query.exec();
  }
  query.finish();

  //if(ok) _db->commit();

  if(ok) qDebug("%s",QString("%1: %2").arg(tr("File imported")).arg(ftitle).toUtf8().data());
  else qWarning("%s",QString("%1: %2").arg(tr("File import error")).arg(ftitle).toUtf8().data());
  return ok?telemetryID:0;
}
//=============================================================================
QByteArray MissionXml::readXmlPart(QXmlStreamReader &xml)
{
  const QString tag=xml.name().toString();
  QByteArray xmlPart;
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
