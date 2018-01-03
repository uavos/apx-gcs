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
#include "VehicleMandala.h"
#include "Vehicle.h"
#include "Mandala.h"
#include "VehicleMandalaFact.h"
#include "Mandala.h"
#include "node.h"
//=============================================================================
VehicleMandala::VehicleMandala(Vehicle *parent)
  : Fact(parent,"mandala","Mandala",tr("Vehicle data tree"),GroupItem,NoData),
    vehicle(parent),
    m_errcnt(0)
{
  m=new Mandala();

  //---------------------
  QString dscString;
#define MIDX(...) \
  dscString.append(#__VA_ARGS__);
#define MVAR(...) \
  dscString.append(#__VA_ARGS__);
#define MBIT(...) \
  dscString.append(#__VA_ARGS__);
#include "MandalaVars.h"

  //unique version ID
  m_md5=QCryptographicHash::hash(QByteArray(dscString.toUtf8()),QCryptographicHash::Md5);

  //sig fields (for request & send)
  for(uint var_idx=0;var_idx<idxPAD;var_idx++){
    uint type;
    void *value_ptr;
    if(!m->get_ptr(var_idx,&value_ptr,&type))break;
    if(type!=vt_idx)continue;
    const char *var_name;
    const char *var_descr;
    if(!m->get_text_names(var_idx|0xFF00,&var_name,&var_descr))continue;
    special.insert(var_name,var_idx);
  }

  //data facts
  for(uint var_idx=idxPAD;var_idx<idx_vars_top;var_idx++){
    uint type;
    void *value_ptr;
    if(!m->get_ptr(var_idx,&value_ptr,&type))break;
    const char *var_name;
    const char *var_descr;
    if(!m->get_text_names(var_idx|0xFF00,&var_name,&var_descr))break;
    const char *name;
    const char *descr;
    switch(type){
      case vt_flag: {
        uint msk=1;
        while((msk<0x0100) && m->get_text_names(var_idx|(msk<<8),&name,&descr)){
          QString dsc=QString(descr).trimmed();
          QString dsc_tr=qApp->translate("MandalaVars",descr);
          QStringList enumStrings;
          if(dsc.contains('/') && dsc.indexOf('/')>dsc.lastIndexOf(' ')){
            // retracted/extracted enum
            QString sFalse=dsc.right(dsc.size()-dsc.indexOf("/")-1).trimmed();
            QString sTrue=dsc.left(dsc.indexOf("/")).trimmed();
            sTrue.remove(0,dsc.lastIndexOf(' ')+1);
            dsc=dsc.left(dsc.lastIndexOf(' '));
            enumStrings<<sFalse.trimmed()<<sTrue.trimmed();
            constants[QString("%1_%2").arg(name).arg(sFalse)]=false;
            constants[QString("%1_%2").arg(name).arg(sTrue)]=true;
          }
          VehicleMandalaFact *f=registerFact(var_idx|(msk<<8),enumStrings.size()?EnumData:BoolData,QString("%1_%2").arg(var_name).arg(name),QString("%1 (%2)").arg(qApp->translate("MandalaVars",descr)).arg(var_name),"bit");
          f->setEnumStrings(enumStrings);
          msk<<=1;
        }
      }break;
      case vt_enum: {
        QStringList enumStrings;
        uint msk=0;
        while(m->get_text_names(var_idx|(msk<<8),&name,&descr)){
          constants[QString("%1_%2").arg(var_name).arg(name)]=msk;
          enumStrings.append(name);
          msk++;
        }
        QString su="{"+enumStrings.join(", ")+"}";
        VehicleMandalaFact *f=registerFact(var_idx,EnumData,var_name,QString("%1 (enum)").arg(qApp->translate("MandalaVars",var_descr)),"enum");
        f->setEnumStrings(enumStrings);
      }break;
      case vt_vect:
      case vt_point: {
        for(int iv=0;iv<((type==vt_vect)?3:2);iv++){
          QString su,dsc=QString(var_descr).trimmed();
          QString dsc_tr=qApp->translate("MandalaVars",var_descr);
          if(dsc.contains('[')){ // roll,pitch,yaw [deg]
            su=dsc.right(dsc.size()-dsc.indexOf("[")).trimmed();
            dsc=dsc.left(dsc.indexOf('[')).trimmed();
            su.remove('[');
            su.remove(']');
            QStringList lsu=su.split(',');
            if(iv<lsu.size())su=lsu.at(iv);
            if(dsc_tr.contains('[')){
              QString su_tr=dsc_tr.right(dsc_tr.size()-dsc_tr.indexOf("[")).trimmed();
              dsc_tr=dsc_tr.left(dsc_tr.indexOf('[')).trimmed();
              su_tr.remove('[');
              su_tr.remove(']');
              QStringList lsu=su_tr.split(',');
              if(iv<lsu.size())su=lsu.at(iv);
              else su=su_tr;
            }
          }
          QString vname(var_name);
          if(dsc.contains(":") && dsc.contains(",")){
            QString prefix=dsc.left(dsc.indexOf(":")).trimmed();
            QStringList vlist=dsc.mid(dsc.indexOf(":")+1).trimmed().split(',');
            if(!vname.contains("_"))vname.clear();
            else vname=vname.left(vname.lastIndexOf("_")+1).trimmed();
            vname+=vlist.at(iv).trimmed();

            if(dsc_tr.contains(":")){
              prefix=dsc_tr.left(dsc_tr.indexOf(":")).trimmed();
              QStringList st=dsc_tr.mid(dsc_tr.indexOf(":")+1).trimmed().split(',');
              if(vlist.size()==st.size())vlist=st;
            }else prefix=dsc_tr;
            dsc_tr=prefix+" ("+vlist.at(iv).trimmed()+")";
          }else{
            qWarning("Mandala descr error: %s",var_descr);
          }
          if(!su.isNull()){
            dsc_tr+=" ["+su+"]";
          }
          registerFact((iv<<8)|var_idx,FloatData,vname,dsc_tr.left(dsc_tr.indexOf('[')).trimmed(),su);
        }
      }break;
      default: {
        QString su,dsc=QString(var_descr).trimmed();
        QString dsc_tr=qApp->translate("MandalaVars",var_descr);
        if(dsc.contains("[")){
          su=dsc.right(dsc.size()-dsc.indexOf("[")).trimmed();
          su.remove('[');
          su.remove(']');
          if(dsc_tr.contains('['))dsc_tr.truncate(dsc_tr.indexOf('['));
          dsc_tr=QString("%1 [%2]").arg(dsc_tr.trimmed()).arg(su);
        }
        registerFact(var_idx,type==vt_float?FloatData:IntData,var_name,dsc_tr.left(dsc_tr.indexOf('[')).trimmed(),su);
      }
    }//switch
  }//for


  connect(this,&VehicleMandala::sendUplink,parent,&Vehicle::sendUplink);

}
VehicleMandala::~VehicleMandala()
{
  delete m;
}
//=============================================================================
VehicleMandalaFact * VehicleMandala::registerFact(quint16 id, DataType dataType, const QString &name, const QString &descr, const QString &units)
{
  VehicleMandalaFact *f=new VehicleMandalaFact(this,m,id,dataType,name,"",descr,units);
  idMap[id]=f;
  allFacts.append(f);
  names.append(name);
  ids.append(id);
  return f;
}
//=============================================================================
//=============================================================================
QByteArray VehicleMandala::md5(void) const
{
  return m_md5;
}
bool VehicleMandala::setMd5(const QByteArray &v)
{
  if(m_md5==v)return false;
  m_md5=v;
  emit md5Changed();
  return true;
}
uint VehicleMandala::errcnt(void) const
{
  return m_errcnt;
}
bool VehicleMandala::setErrcnt(const uint &v)
{
  if(m_errcnt==v)return false;
  m_errcnt=v;
  m->dl_errcnt=v;
  emit errcntChanged();
  return true;
}
//=============================================================================
QVariant VehicleMandala::valueById(quint16 id) const
{
  return factById(id)->value();
}
bool VehicleMandala::setValueById(quint16 id,const QVariant &v)
{
  return factById(id)->setValue(v);
}
VehicleMandalaFact * VehicleMandala::factById(quint16 id) const
{
  return idMap.value(id);
}
QVariant VehicleMandala::valueByName(const QString &vname) const
{
  return factByName(vname)->value();
}
VehicleMandalaFact * VehicleMandala::factByName(const QString &vname) const
{
  return static_cast<VehicleMandalaFact*>(child(vname));
}
//=============================================================================
bool VehicleMandala::unpackXPDR(const QByteArray &ba)
{
  uint data_cnt=ba.size();
  if(data_cnt<=bus_packet_size_hdr)return false;
  data_cnt-=bus_packet_size_hdr;
  if(data_cnt!=sizeof(IDENT::_xpdr))return false;
  _bus_packet &packet=*(_bus_packet*)ba.data();
  if(packet.id!=idx_xpdr) return false;
  IDENT::_xpdr *xpdr=(IDENT::_xpdr*)packet.data;
  factById(idx_gps_pos|(0<<8))->setValueLocal(xpdr->lat);
  factById(idx_gps_pos|(1<<8))->setValueLocal(xpdr->lon);
  factById(idx_altitude)->setValueLocal(xpdr->alt);
  factById(idx_gSpeed)->setValueLocal(xpdr->gSpeed/100.0);
  double crs=xpdr->crs*(180.0/32768.0);
  factById(idx_course)->setValueLocal(crs);
  factById(idx_theta|(2<<8))->setValueLocal(crs);
  factById(idx_mode)->setValueLocal(xpdr->mode);
  return true;
}
//=============================================================================
bool VehicleMandala::unpackData(const QByteArray &ba)
{
  _bus_packet &packet=*(_bus_packet*)ba.data();
  uint data_cnt=ba.size();
  if(data_cnt<=bus_packet_size_hdr)return false;
  data_cnt-=bus_packet_size_hdr;
  switch (packet.id) {
    default: break;
    case idx_data:  //serial data
      if(data_cnt<2){
        qWarning("Mandala: %s",tr("Received serial data").toUtf8().data());
        return true;
      }
      emit serialReceived(packet.data[0],QByteArray((const char*)(packet.data+1),data_cnt-1));
      return true;
  }
  if(!m->unpack(packet.data,data_cnt,packet.id)) return false;
  //update fact
  VehicleMandalaFact *fact=NULL;
  if(packet.id==idx_set){
    fact=idMap.value((uint16_t)packet.data[0]|(uint16_t)packet.data[1]<<8);
  }/*else{
    fact=idMap.value(packet.id);
  }*/
  //if(fact)fact->loadValue();
  for(int i=0;i<allFacts.size();++i){
    allFacts.at(i)->loadValue();
  }
  emit dataReceived(fact?fact->id():packet.id);
  return true;
}
//=============================================================================
bool VehicleMandala::unpackTelemetry(const QByteArray &ba)
{
  _bus_packet &packet=*(_bus_packet*)ba.data();
  uint data_cnt=ba.size();
  if(data_cnt<bus_packet_size_hdr)return false;
  data_cnt-=bus_packet_size_hdr;
  if(data_cnt<4)return false;
  if(packet.id!=idx_downstream) return false;
  if(!m->extract_downstream(packet.data,data_cnt)) return false;
  //load facts
  foreach (VehicleMandalaFact *f, idMap.values()) {
    f->loadValue();
  }
  setErrcnt(m->dl_errcnt);
  emit dataReceived(packet.id);
  return true;
}
//=============================================================================
