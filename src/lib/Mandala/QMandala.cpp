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
#include "FactSystem.h"
//=============================================================================
QMandala *QMandala::_instance=NULL;
//=============================================================================
QMandala::QMandala()
 : current(NULL)
{
  _instance=this;
  setObjectName("mandala");
  qApp->setProperty("Mandala",qVariantFromValue(this));

  //---------------------
  //create missing dirs
  //if(!Global::config().exists()) Global::config().mkpath(".");
  //if(!Global::missions().exists())Global::missions().mkpath(".");
  //if(!Global::scr().exists()) Global::scr().mkpath(".");
  //if(!Global::records().exists())Global::records().mkpath(".");


  //properties
  m_jsValid=false;
  m_size=0;

  prevUAV=NULL;
  //local mandala
  local=new QMandalaItem(this);
  strncat(local->ident.callsign,"LOCAL",sizeof(local->ident.callsign));
  setCurrent(local);
  connect(local,SIGNAL(sendUplink(QByteArray)),this,SLOT(uavSendUplink(QByteArray)));
  emit uavAdded(local);

  //makeTree(local);

  //dlink request timer
  connect(&dlinkReqTimer,SIGNAL(timeout()),this,SLOT(dlinkReqTimeout()));
  dlinkReqTimer.setInterval(1000);
  //ident request timer
  connect(&identReqTimer,SIGNAL(timeout()),this,SLOT(identReqTimeout()));
  identReqTimer.setInterval(1000);
  identReqTimer.setSingleShot(true);
}
//=============================================================================
const MandalaCore::_vars_list QMandala::vars_gcu = { idx_gcu_RSS, idx_gcu_Ve, idx_gcu_MT,0};
//=============================================================================
void QMandala::downlinkReceived(const QByteArray &ba)
{
  _bus_packet *packet=(_bus_packet*)ba.data();
  uint data_cnt=ba.size();
  if(data_cnt<=bus_packet_size_hdr)return;
  data_cnt-=bus_packet_size_hdr;
  switch(packet->id){
    case idx_xpdr:{      //transponder from UAV received
      if(data_cnt!=sizeof(IDENT::_xpdr))return;
      IDENT::_xpdr *xpdr=(IDENT::_xpdr*)packet->data;
      //qDebug()<<"xpdr"<<xpdr->squawk;
      //find uav in list by squawk
      if(xpdr->squawk){
        foreach(QMandalaItem *m,items){
          if(m->ident.squawk!=xpdr->squawk)continue;
          m->downlinkReceived(ba);
          return;
        }
      }
      //new transponder detected, request IDENT
      reqIDENT(xpdr->squawk);
    }return;
    case idx_ident:{
      if(data_cnt!=sizeof(IDENT::_ident))return;
      IDENT::_ident *ident=(IDENT::_ident*)packet->data;
      //qDebug()<<"ident"<<ident->callsign;
      if(!ident->squawk){
        assignIDENT(ident);
        return;
      }
      //find uav in list by ident, exit when found
      foreach(QMandalaItem *m,items){
        if(m->ident.squawk!=ident->squawk)continue;
        //squawk match, check uid
        if(memcmp(&m->ident.uid,ident->uid,sizeof(IDENT::_uid))!=0){
          //same squawk on different uid => duplicate squawk
          //qDebug("QMandala %s %s: %s (%.4X)",tr("Assigning squawk to").toUtf8().data(),vclassToString(ident->vclass).toUtf8().data(),ident->callsign,ident->squawk);
          assignIDENT(ident);
          return;
        }
        if(ident->callsign[0] && memchr(ident->callsign,0,sizeof(IDENT::_callsign)) && strcmp(ident->callsign,m->ident.callsign)!=0){
          memcpy(m->ident.callsign,ident->callsign,sizeof(IDENT::_callsign));
          if(m==current)setUavName(QString(ident->callsign));
          emit uavNameChanged(uavName());
        }
        if(m->ident.vclass!=ident->vclass){
          //vehicle class was updated?
          m->ident.vclass=ident->vclass;
          continue;
        }
        if(ident->vclass==IDENT::GCU)return; //
        //check whole ident match
        if(memcmp(&m->ident,ident,sizeof(IDENT::_ident))!=0)continue;
        return;
      }
      //new UAV ident found
      QMandalaItem *m=append();
      memcpy(&m->ident,ident,sizeof(IDENT::_ident));
      //qDebug("QMandala %s %s: %s (%.4X)",vclassToString(ident->vclass).toUtf8().data(),tr("identified").toUtf8().data(),m->ident.callsign,m->ident.squawk);
      emit uavAdded(m);
      if(size()==1) setCurrent(m);
      else if(isGCU() && (m->ident.vclass!=IDENT::GCU)) setCurrent(m);
    }return;
    case idx_dlink:{
      if(data_cnt<=sizeof(IDENT::_squawk))return;
      IDENT::_squawk squawk=packet->data[0]|packet->data[1]<<8;
      //find uav in list by squawk
      QByteArray pdata(ba.right(data_cnt-sizeof(IDENT::_squawk)));
      packet=(_bus_packet*)pdata.data();
      if(!squawk)return;
      //check if new transponder detected, request IDENT
      bool bFound=false;
      foreach(QMandalaItem *m,items){
        if(m->ident.squawk!=squawk)continue;
        bFound=true;
        break;
      }
      if(!bFound) reqIDENT(squawk);
      //process received dlink packet
      if(packet->id && strchr((const char*)vars_gcu,packet->id)!=NULL){
        //GCU RSS var arrived - broadcast to all mandalas
        local->downlinkReceived(pdata);
        foreach(QMandalaItem *m,items) m->downlinkReceived(pdata);
      }else if(packet->id==idx_hid){
        //joystick jsw panel
        const QByteArray &jp=pdata.right(pdata.size()-bus_packet_size_hdr);
        if(jp.size())uavSendUplink(jp);
        //qDebug()<<"hid:"<<jp.toHex();
      }else if(packet->id==idx_jsexec){
        //joystick jsw panel qtscript
        QByteArray sba((const char*)packet->data,pdata.size()-bus_packet_size_hdr);
        FactSystem::instance()->jsexec(QString(sba));
        //qDebug()<<"js:"<<sba.toHex();
      }else{
        if(packet->id==idx_service && packet->srv.cmd!=apc_msg){
          //forward all service to local
          local->downlinkReceived(pdata);
        }
        foreach(QMandalaItem *m,items){
          //data for particular mandala received
          if(m->ident.squawk!=squawk)continue;
          m->downlinkReceived(pdata);
          break;
        }
      }
    }return;
  }
  //local var received (no dlink)
  if(packet->id && strchr((const char*)vars_gcu,packet->id)!=NULL){
    local->downlinkReceived(ba);
    //GCU RSS var arrived - broadcast to all mandalas
    foreach(QMandalaItem *m,items) m->downlinkReceived(ba);
  }else if(packet->id==idx_hid){
    //joystick jsw panel
    const QByteArray &jp=ba.right(ba.size()-bus_packet_size_hdr);
    if(jp.size())uavSendUplink(jp);
    //qDebug()<<"hid:"<<jp.toHex();
  }else if(packet->id==idx_jsexec){
    //joystick jsw panel qtscript
    QByteArray sba((const char*)packet->data,data_cnt);
    FactSystem::instance()->jsexec(QString(sba));
    //qDebug()<<"js:"<<sba.toHex();
  }else local->downlinkReceived(ba);
}
//=============================================================================
#define VEHICLE_CLASS_LIST_STR_IMPL2(...) #__VA_ARGS__
#define VEHICLE_CLASS_LIST_STR_IMPL(...) VEHICLE_CLASS_LIST_STR_IMPL2(__VA_ARGS__)
#define VEHICLE_CLASS_LIST_STR VEHICLE_CLASS_LIST_STR_IMPL(VEHICLE_CLASS_LIST)
QString QMandala::vclassToString(IDENT::_vclass vclass)
{
  QStringList st=QString(VEHICLE_CLASS_LIST_STR).split(',');
  if(vclass>=st.size())return tr("UAV");
  return st.at(vclass);
}
//=============================================================================
void QMandala::reqIDENT(IDENT::_squawk squawk)
{return;
  QByteArray ba=QByteArray().append((unsigned char)idx_ident).append((unsigned char)squawk).append((unsigned char)(squawk>>8));
  if(!req_list.contains(ba)){
    req_list.append(ba);
    identReqTimer.start();
  }
}
void QMandala::assignIDENT(IDENT::_ident *ident)
{return;
  //fix and update ident
  uint tcnt=1000000;
  while(tcnt--){
    ident->squawk=qrand()+tcnt;
    if(ident->squawk<100||ident->squawk>0xff00)continue;
    bool bFound=false;
    foreach(QMandalaItem *m,items){
      if(ident->squawk==m->ident.squawk){
        bFound=true;
        break;
      }
    }
    if(bFound)continue;
    break;
  }
  if(!tcnt){
    qWarning("%s",tr("Can't find new squawk for assignment").toUtf8().data());
    return;
  }
  //unique squawk assigned, update callsign
  if(strnlen((const char*)ident->callsign,sizeof(IDENT::_callsign))<=0){
    snprintf((char*)ident->callsign,sizeof(IDENT::_callsign),(ident->vclass==IDENT::GCU)?"UAVOS-GCU-%X":"UAVOS-%X",ident->squawk);
  }
  //send new ident
  QByteArray ba=QByteArray().append((unsigned char)idx_ident);
  int sz=ba.size();
  ba.resize(sz+sizeof(IDENT::_ident));
  memcpy(ba.data()+sz,ident,sizeof(IDENT::_ident));
  emit sendUplink(ba);
}
//=============================================================================
void QMandala::testUAV()
{
  QMandalaItem *m=append();
  strncat(m->ident.callsign,"X-test UAV",sizeof(m->ident.callsign));
  m->setObjectName("test");
  emit uavAdded(m);
  setCurrent(m);
}
//=============================================================================
void QMandala::uavSendUplink(const QByteArray &ba)
{
  if(isLocal()){
    emit sendUplink(ba);
    return;
  }
  //prepend idx_dlink+squawk
  emit sendUplink(QByteArray().append((unsigned char)idx_dlink).append((unsigned char)current->ident.squawk).append((unsigned char)(current->ident.squawk>>8)).append(ba));
}
//=============================================================================
void QMandala::dlinkReqTimeout(void)
{
  emit sendUplink(QByteArray().append((unsigned char)idx_dlink).append((unsigned char)current->ident.squawk).append((unsigned char)(current->ident.squawk>>8)));
}
void QMandala::identReqTimeout(void)
{
  if(!req_list.size())return;
  emit sendUplink(req_list.takeFirst());
  identReqTimer.start();
}
//=============================================================================
//=============================================================================
QMandalaItem *QMandala::append(void)
{
  QMandalaItem *m=new QMandalaItem(this);
  connect(m,SIGNAL(sendUplink(QByteArray)),this,SLOT(uavSendUplink(QByteArray)));
  items.append(m);
  setSize(items.size());
  return m;
}
//=============================================================================
bool QMandala::checkSquawk(QMandalaItem *m_current, QMandalaItem *m, bool silent)
{
  //check for duplicate squawk
  if(m==local)return true;
  while(1){
    if(m_current){
      if(m_current!=local && m_current->ident.squawk==m->ident.squawk) break;
    }else{
      foreach(QMandalaItem *m_current,items){
        if(m_current!=local && m!=m_current && m_current->ident.squawk==m->ident.squawk)
          break;
      }
    }
    return true;
  }
  if(!silent)
    qWarning("%s (%.4X): %s/%s",tr("Duplicate squawk").toUtf8().data(),m->ident.squawk,m_current->ident.callsign,m->ident.callsign);
  return false;
}
//=============================================================================
void QMandala::setCurrent(QMandalaItem *m)
{
  if(m!=local && (!items.contains(m))){
    qWarning("%s: %s",tr("UAV not found").toUtf8().data(),m->ident.callsign);
    return;
  }
  bool wasLocal=false;
  if(current){ //there was an UAV selected before
    qDebug("!%s %s: %s",vclassToString(m->ident.vclass).toUtf8().data(),tr("selected").toUtf8().data(),m->ident.callsign);
    if(m==current)return;
    current->setActive(false);
    disconnect(current,SIGNAL(dlinkDataChanged(bool)),this,SIGNAL(dlinkDataChanged(bool)));
    disconnect(current,SIGNAL(xpdrDataChanged(bool)),this,SIGNAL(xpdrDataChanged(bool)));
    disconnect(current,SIGNAL(replayDataChanged(bool)),this,SIGNAL(replayDataChanged(bool)));
    wasLocal=isLocal();
    checkSquawk(current,m);
  }
  reqIDENT(m->ident.squawk);
  //set current
  current=m;
  setUavName(QString(m->ident.callsign));
  if((!isGCU())&&(!isLocal())) prevUAV=m;
  emit currentChanged(current);
  if(isLocal()!=wasLocal)emit isLocalChanged(isLocal());
  //propagate props stored in uav mandala
  connect(current,SIGNAL(dlinkDataChanged(bool)),this,SIGNAL(dlinkDataChanged(bool)));
  connect(current,SIGNAL(xpdrDataChanged(bool)),this,SIGNAL(xpdrDataChanged(bool)));
  connect(current,SIGNAL(replayDataChanged(bool)),this,SIGNAL(replayDataChanged(bool)));
  m->setActive(true);
  emit errcntChanged(errcnt());
  emit dlinkDataChanged(dlinkData());
  emit xpdrDataChanged(xpdrData());
  emit replayDataChanged(replayData());
  current->emitUpdated();
}
void QMandala::setCurrent(QString callsign)
{
  if(callsign==QString(local->ident.callsign)){
    setCurrent(local);
    return;
  }
  foreach(QMandalaItem* m,items){
    if(QString(m->ident.callsign)!=callsign)continue;
    setCurrent(m);
    return;
  }
  qWarning("%s: %s",tr("UAV not found").toUtf8().data(),callsign.toUtf8().data());
}
void QMandala::setCurrentGCU(void)
{
  foreach(QMandalaItem* m,items){
    if(m->ident.vclass!=IDENT::GCU)continue;
    setCurrent(m);
    return;
  }
  qWarning("%s",tr("GCU not found").toUtf8().data());
  setCurrent(local);
}
void QMandala::changeCurrent(void)
{
  if(isGCU()){
    if(prevUAV){
      setCurrent(prevUAV);
      return;
    }
    if(local->alive()){
      setCurrent(local);
      return;
    }
  }
  if(isLocal() && prevUAV){
    setCurrent(prevUAV);
    return;
  }
  setCurrentGCU();
}
void QMandala::changeCurrentNext(void)
{
  if(items.isEmpty()){
    setCurrent(local);
    return;
  }
  int i=items.indexOf(current)+1;
  if(i>=items.size())i=0;
  setCurrent(items.at(i));
}
void QMandala::changeCurrentPrev(void)
{
  if(items.isEmpty()){
    setCurrent(local);
    return;
  }
  int i=items.indexOf(current)-1;
  if(i<0)i=items.size()-1;
  setCurrent(items.at(i));
}
//=============================================================================
//=============================================================================
bool QMandala::jsValid()
{
  return m_jsValid;
}
void QMandala::setJsValid(bool v)
{
  if(m_jsValid==v)return;
  m_jsValid=v;
  emit jsChanged(v);
}
bool QMandala::dlinkData()
{
  return current->dlinkData();
}
bool QMandala::xpdrData()
{
  return current->xpdrData();
}
bool QMandala::replayData()
{
  return current->replayData();
}
uint QMandala::errcnt()
{
  return current->dl_errcnt;
}
void QMandala::setErrcnt(uint v)
{
  if(current->dl_errcnt==v)return;
  current->dl_errcnt=v;
  emit errcntChanged(v);
}
QString QMandala::uavName()
{
  return m_uavName;
}
void QMandala::setUavName(QString v)
{
  if(m_uavName==v)return;
  m_uavName=v;
  emit uavNameChanged(v);
}
bool QMandala::isLocal()
{
  return current==local;
}
bool QMandala::isGCU()
{
  return current->ident.vclass==IDENT::GCU;
}
uint QMandala::size()
{
  return m_size;
}
void QMandala::setSize(uint v)
{
  if(m_size==v)return;
  m_size=v;
  if(v)dlinkReqTimer.start();
  emit sizeChanged(v);
  //qDebug()<<"QMandala::sizeChanged"<<v;
}
QStringList QMandala::uavNames()
{
  QStringList st;
  st.append(local->ident.callsign);
  foreach(QMandalaItem* m,items){
    st.append(m->ident.callsign);
  }
  return st;
}
QMandalaItem * QMandala::mcurrent()
{
  return current;
}
//=============================================================================

