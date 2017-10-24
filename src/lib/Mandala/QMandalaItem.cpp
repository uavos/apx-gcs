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
#include <QToolTip>
#include "QMandala.h"
#include "AppSettings.h"
#include "FactSystem.h"
#include "AppDirs.h"
//=============================================================================
//=============================================================================
QMandalaItem::QMandalaItem(QObject *parent,bool bbox)
:QObject(parent), Mandala(),bbox(bbox),datalinkReadonly(NULL)
{
  setObjectName("mandala");

  memset(&ident,0,sizeof(ident));
  m_dlinkData=false;
  m_xpdrData=false;
  m_replayData=false;
  m_active=false;

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
  md5=QCryptographicHash::hash(QByteArray(dscString.toUtf8()),QCryptographicHash::Md5);


  //fields
  field_void=new QMandalaField();
  for(uint var_idx=idxPAD;var_idx<idx_vars_top;var_idx++){
    uint type;
    void *value_ptr;
    if(!get_ptr(var_idx,&value_ptr,&type))break;
    const char *var_name;
    const char *var_descr;
    if(!get_text_names(var_idx|0xFF00,&var_name,&var_descr))break;
    const char *name;
    const char *descr;
    switch(type){
      case vt_flag: {
        uint m=1;
        while((m<0x0100)&&get_text_names(var_idx|(m<<8),&name,&descr)){
          registerField(var_idx|(m<<8),QString("%1_%2").arg(var_name).arg(name),QString("%1 (%2) [0/1]").arg(qApp->translate("MandalaVars",descr)).arg(var_name),"bit");
          m<<=1;
        }
      }break;
      case vt_enum: {
        QStringList stu;
        uint m=0;
        while(get_text_names(var_idx|(m<<8),&name,&descr)){
          constants[QString("%1_%2").arg(var_name).arg(name)]=m;
          enumnames[var_idx].append(qApp->translate("MandalaVars",name));
          stu.append(name);
          m++;
        }
        QString su="{"+stu.join(", ")+"}";
        //src_vars_dsc[var_idx]=QString("%1 (enum) [%2]").arg(descr).arg(su);
        registerField(var_idx,var_name,QString("%1 (enum) [%2]").arg(qApp->translate("MandalaVars",var_descr)).arg(su),su);
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
          registerField((iv<<8)|var_idx,vname,dsc_tr.trimmed(),su);
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
        registerField(var_idx,var_name,dsc_tr,su);
      }

    }//switch

  }//for

  //-------------
  rec=new FlightDataFile(this);

  if(bbox)return;

  //Scrpting
  QScriptValue mobj=engine.newQObject(this,QScriptEngine::QtOwnership,QScriptEngine::ExcludeChildObjects|QScriptEngine::ExcludeSuperClassMethods|QScriptEngine::ExcludeSuperClassProperties);
  engine.globalObject().setProperty("mandala",mobj);
  foreach(QMandalaField *f,fields)
    mobj.setProperty(f->name(),engine.newQObject(f,QScriptEngine::QtOwnership,QScriptEngine::ExcludeChildObjects|QScriptEngine::ExcludeSuperClassMethods|QScriptEngine::ExcludeSuperClassProperties),QScriptValue::Undeletable);

  mobj.setProperty("global",engine.newQObject(QMandala::instance(),QScriptEngine::QtOwnership,QScriptEngine::ExcludeChildObjects|QScriptEngine::ExcludeSuperClassMethods|QScriptEngine::ExcludeSuperClassProperties),QScriptValue::Undeletable);

  //sig fields (for request & send)
  for(uint var_idx=0;var_idx<idxPAD;var_idx++){
    uint type;
    void *value_ptr;
    if(!get_ptr(var_idx,&value_ptr,&type))break;
    if(type!=vt_idx)continue;
    const char *var_name;
    const char *var_descr;
    if(!get_text_names(var_idx|0xFF00,&var_name,&var_descr))continue;
    QMandalaField *f=new QMandalaField(this,var_idx,var_name,var_descr,"raw");
    mobj.setProperty(f->name(),engine.newQObject(f,QScriptEngine::QtOwnership,QScriptEngine::ExcludeChildObjects|QScriptEngine::ExcludeSuperClassMethods|QScriptEngine::ExcludeSuperClassProperties),QScriptValue::Undeletable);
  }

  //short var access in 'this'
  foreach(QMandalaField *f,fields){
    exec_script(QString("__defineGetter__(\"%1\", function() { return mandala.%1.value; });").arg(f->name()));
    exec_script(QString("__defineSetter__(\"%1\", function(v) { mandala.%1.setValue(v); });").arg(f->name()));
  }

  //system functions and objects
  add_scr("help",QObject::tr("print commands help"),"mandala.scr_help();");
  add_scr("set(n,v)",QObject::tr("set var n to value v and send to UAV"),"if(arguments.length==2)mandala[n].setValue(v);else mandala[n].setValue(!mandala[n].value);");
  add_scr("req(n)",QObject::tr("request var n from UAV"),"mandala[n].request();");
  add_scr("send(n)",QObject::tr("send var n to UAV"),"mandala[n].send();");
  add_scr("nodes",QObject::tr("rescan bus nodes"),"print('nodes:');mandala.req_nodes();");
  add_scr("nstat",QObject::tr("print nodes status"),"print('nodes statistics:');mandala.req_nstat();");
  add_scr("serial(p,v)",QObject::tr("send data v to serial port ID p"),"mandala.scr_serial(p,v);");
  add_scr("can(i,e,v)",QObject::tr("send data v to CAN ID i with IDE e"),"mandala.scr_can(i,e,v);");
  add_scr("vmexec(f)",QObject::tr("execute function on VMs"),"mandala.scr_vmexec(f);");
  add_scr("sleep(n)",QObject::tr("sleep n milliseconds"),"mandala.scr_sleep(n);");
  add_scr("next()",QObject::tr("switch to next vehicle"),"mandala.global.changeCurrentNext();");
  add_scr("prev()",QObject::tr("switch to previous vehicle"),"mandala.global.changeCurrentPrev();");
  //some helper functions
  add_scr("trigger(v,a,b)",QObject::tr("trigger value of v to a or b"),"if(v==a)return b; else return a;");
  add_scr("bound(v)",QObject::tr("wrap angle -180..+180"),"while(v>=180)v-=360;while(v<-180)v+=360;return v;");
  add_scr("ls(a,b)",QObject::tr("print members of type b for scope a"),"for(var i in a)if(typeof(a[i])==b || !b)print(i+\" - \"+typeof(a[i]));");
  add_scr("vars(a)",QObject::tr("print variables for scope a"),"if(arguments.length==0)a=this;for(var i in a)if(typeof(a[i])=='number')print(i+\"=\"+a[i]);");
  add_scr("func(a)",QObject::tr("print functions for scope a"),"if(arguments.length==0)a=this;for(var i in a)if(typeof(a[i])=='function')print(i);");
  //predefined commands
  add_scr("ahrs",QObject::tr("reset AHRS"),"req('roll');");
  add_scr("zrc",QObject::tr("reset pilot controls"),"rc_roll=0;rc_pitch=0;rc_throttle=0;rc_yaw=0;");
  add_scr("zps",QObject::tr("reset barometric altitude on ground"),"altps_gnd=0;");
  add_scr("ned",QObject::tr("reset local GPS coordinates"),"home_lat=gps_lat;home_lon=gps_lon;home_hmsl=gps_hmsl;");
  add_scr("hmsl",QObject::tr("reset local GPS altitude"),"home_hmsl=gps_hmsl;");

  //constants
  foreach(QString name,constants.keys())
    engine.globalObject().setProperty(name,constants.value(name),QScriptValue::ReadOnly|QScriptValue::Undeletable);

  // script include file (default)
  QFile jsFile(AppDirs::res().filePath("scripts/gcs.js"));
  if (jsFile.open(QIODevice::ReadOnly)){
    QTextStream stream(&jsFile);
    QString contents = stream.readAll();
    jsFile.close();
    engine.evaluate(contents,jsFile.fileName());
  }
  // script include file (user commands)
  QFile jsFile2(AppDirs::scripts().filePath("gcs.js"));
  if (jsFile2.open(QIODevice::ReadOnly)){
    QTextStream stream(&jsFile2);
    QString contents = stream.readAll();
    jsFile2.close();
    engine.evaluate(contents,jsFile2.fileName());
  }


  //timeouts
  dlinkDataTimer.setSingleShot(true);
  dlinkDataTimer.setInterval(3000);
  connect(&dlinkDataTimer,SIGNAL(timeout()),SLOT(dlinkDataTimeout()));
  xpdrDataTimer.setSingleShot(true);
  xpdrDataTimer.setInterval(3000);
  connect(&xpdrDataTimer,SIGNAL(timeout()),SLOT(xpdrDataTimeout()));
  replayDataTimer.setSingleShot(true);
  replayDataTimer.setInterval(3000);
  connect(&replayDataTimer,SIGNAL(timeout()),SLOT(replayDataTimeout()));

  //request seq
  connect(&nstatTimer,SIGNAL(timeout()),this,SLOT(req_nstat_next()));
}
QMandalaItem::~QMandalaItem()
{
  qDeleteAll(node_info);
}
//=============================================================================
void QMandalaItem::registerField(uint varmsk,QString name,QString descr,QString units)
{
  names.append(name);
  QMandalaField *f=new QMandalaField(this,varmsk,name,descr,units);
  fields.append(f);
  fieldsByName.insert(f->name(),f);
  fieldsByVarmsk.insert(varmsk,f);
}
//=============================================================================
QMandalaField *QMandalaItem::field(uint varmsk)
{
  if(!varmsk)return field_void;
  if(fieldsByVarmsk.contains(varmsk)) return fieldsByVarmsk.value(varmsk);
  //if(varmsk)qWarning("Mandala field idx not found: %u",varmsk);
  return field_void;
}
QMandalaField *QMandalaItem::field(const QString &name)
{
  if((!name.size())||name=="undefined")return field_void;
  if(fieldsByName.contains(name)) return fieldsByName.value(name);
  //if(name.size())qWarning("Mandala field not found: %s",name.toUtf8().data());
  return field_void;
}
QString QMandalaItem::enumName(uint8_t varidx,int value) const
{
  const QStringList &st=enumnames.value(varidx);
  return (value>=0 && value<st.size())?st.at(value):QString();
}
//=============================================================================
//=============================================================================
void QMandalaItem::add_scr(QString name, QString description, QString body)
{
  QString fn=name;
  if(fn.contains('('))fn.remove(fn.indexOf('('),fn.size());
  if(fieldsByName.contains(fn)){
    qWarning("%s: %s",tr("Function not registered").toUtf8().data(),name.toUtf8().data());
    return;
  }
  fn=name;
  if(!fn.contains("("))fn.append("()");
  exec_script(QString("function %1 { %2;};").arg(fn).arg(body));
  scr_descr[name]=description;
}
//=============================================================================
void QMandalaItem::exec_script(const QString &script)
{
  //qDebug()<<script;
  QScriptSyntaxCheckResult chk=QScriptEngine::checkSyntax(script);
  if(chk.state()!=QScriptSyntaxCheckResult::Valid){
    qWarning("%s: %s",tr("Syntax error").toUtf8().data(),chk.errorMessage().toUtf8().data());
    return;
  }
  engine.evaluate(script);
  if(engine.hasUncaughtException()){
    qWarning()<<script;
    qWarning("%s",engine.uncaughtException().toString().toUtf8().data());
  }
}
//=============================================================================
void QMandalaItem::scr_help()
{
  QString s;
  s+="<html><table width=100%>";
  foreach(const QString &cmd,scr_descr.keys()){
    s+="<tr><td valign='middle' style='background-color: #111; font-family: Monospace;'>";
    s+="<NOBR>&nbsp;<font color=cyan>";
    s+=cmd;
    s+="</font></NOBR></td><td width='100%'>";
    s+="<font color=gray> &nbsp;"+scr_descr.value(cmd)+"</font>";
    s+="</td></tr>";
  }
  s+="</table></font>";
  qDebug("%s",s.toUtf8().data());
}
//=============================================================================
void QMandalaItem::scr_serial(QScriptValue portID,QScriptValue data)
{
  bool bOK=false;
  uint pID=0;
  QByteArray ba;
  while(portID.isNumber()){
    pID=portID.toInteger();
    if(pID>255)break;
    ba=scrToArray(data);
    bOK=ba.size();
    break;
  }
  if(!bOK){
    qWarning("%s: %s, %s",tr("Can't parse command parameters").toUtf8().data(),portID.toString().toUtf8().data(),data.toString().toUtf8().data());
    return;
  }
  send_serial(pID,ba);
}
//=============================================================================
void QMandalaItem::scr_can(QScriptValue can_ID,QScriptValue can_IDE,QScriptValue data)
{
  bool bOK=false;
  uint vID=0;
  uint vIDE=0;
  QByteArray ba;
  while(can_ID.isNumber()&&can_IDE.isNumber()){
    vID=can_ID.toInteger();
    vIDE=can_IDE.toInteger();
    if(vIDE>1)break;
    ba=scrToArray(data);
    bOK=ba.size();
    break;
  }
  if(!bOK){
    qWarning("%s: %s, %s, %s",tr("Can't parse command parameters").toUtf8().data(),can_ID.toString().toUtf8().data(),can_IDE.toString().toUtf8().data(),data.toString().toUtf8().data());
    return;
  }
  send_can(vID,vIDE,ba);
}
//=============================================================================
void QMandalaItem::scr_vmexec(QScriptValue func)
{
  bool bOK=false;
  QString s=func.toString();
  bOK=s.startsWith('@');
  if(!bOK){
    qWarning("%s: %s",tr("Can't parse command parameters").toUtf8().data(),func.toString().toUtf8().data());
    return;
  }
  send_vmexec(s);
}
//=============================================================================
void QMandalaItem::scr_sleep(uint ms)
{
  QEventLoop loop;
  QTimer::singleShot(ms,&loop,SLOT(quit()));
  loop.exec();
}
//=============================================================================
QByteArray QMandalaItem::scrToArray(QScriptValue data)
{
  QByteArray ba;
  if(data.isString() || data.toString().contains(',')){
    bool ok=false;
    foreach(QString sv,data.toString().trimmed().toLower().split(',',QString::SkipEmptyParts)){
      uint v;
      sv=sv.trimmed();
      if(sv.startsWith("0x"))v=sv.mid(2).toInt(&ok,16);
      else v=sv.toInt(&ok,10);
      if(!ok)break;
      ba.append((unsigned char)v);
    }
    if(!ok)return QByteArray();
  }else if(data.isNumber()){
    ba.append((unsigned char)data.toInteger());
  }else return QByteArray();
  return ba;
}
//=============================================================================
void QMandalaItem::send(unsigned char var_idx, const QByteArray &data)
{
  if(!datalinkReadonly){
    datalinkReadonly=FactSystem::instance()->tree()->fact("datalink.readonly");
  }
  if((!datalinkReadonly) || datalinkReadonly->value().toBool())return;
  QByteArray ba;
  ba.append((unsigned char)var_idx);
  ba.append(data);
  emit sendUplink(ba);
  rec->record_uplink(ba);
  //qDebug()<<ba.toHex();
}
void QMandalaItem::send(unsigned char var_idx)
{
  QByteArray ba;
  ba.resize(sizeof(_bus_packet));
  ba.resize(archive((uint8_t*)ba.data(),ba.size(),var_idx));
  send(var_idx,ba);
}
void QMandalaItem::request(uint var_idx)
{
  send(var_idx,QByteArray());
}
void QMandalaItem::send_srv(uint cmd, const QByteArray &sn, const QByteArray &data)
{
  send(idx_service,QByteArray().append((sn.size()==sizeof(_node_sn))?sn:QByteArray(sizeof(_node_sn),(char)0)).append((char)cmd).append(data));
}
void QMandalaItem::send_serial(uint portNo,const QByteArray &data)
{
  QByteArray ba;
  ba.append((unsigned char)portNo);
  ba.append(data);
  send(idx_data,ba);
  //dump((uint8_t*)ba.data(),ba.size());
}
void QMandalaItem::send_can(uint can_ID, bool can_IDE, const QByteArray &data)
{
  if(data.size()>8)return;
  QByteArray ba;
  ba.append((unsigned char)can_ID&0xFF);
  ba.append((unsigned char)(can_ID>>8)&0xFF);
  ba.append((unsigned char)(can_ID>>16)&0xFF);
  ba.append((unsigned char)(can_ID>>24)&0xFF);
  ba.append((unsigned char)((can_IDE?0x80:0)|data.size())&0xFF);
  ba.append(data);
  send(idx_can,ba);
}
void QMandalaItem::send_vmexec(const QString &func)
{
  send(idx_vmexec,QByteArray().append(func));
}
//=============================================================================
void QMandalaItem::downlinkReceived(const QByteArray &ba)
{
  rec->record_downlink(ba);
  _bus_packet &packet=*(_bus_packet*)ba.data();
  uint data_cnt=ba.size();
  if(data_cnt<=bus_packet_size_hdr)return;
  //if(packet.id==idx_sim)return;
  if(packet.id==idx_service){
    //service packet received
    if(bbox)return;
    if(data_cnt<bus_packet_size_hdr_srv)return;
    data_cnt-=bus_packet_size_hdr_srv;
    QByteArray sn((const char*)packet.srv.sn,sizeof(_node_sn));
    if(isBroadcast(sn))return;
    //const char * name=node_name(sn);
    switch(packet.srv.cmd){
      case apc_search:
        if(!data_cnt){
          if(QMandala::instance()->current==this){
            if(node_info.contains(sn))break;
            send_srv(apc_info,sn);
          }
          return;
        }
        //fallback to old node versions (broadcast reply with _node_info)
        packet.srv.cmd=apc_info;
      case apc_info: {
        //fill available nodes
        if(!data_cnt)return;  //request
        if(data_cnt<sizeof(_node_info)){
          memset(packet.srv.data+data_cnt,0,sizeof(_node_info)-data_cnt);
        }
        _node_info *node=new _node_info;
        memcpy(node,packet.srv.data,sizeof(_node_info));
        bool bNewNode=!node_info.contains(sn);
        if(bNewNode || memcmp(node,node_info.value(sn),sizeof(_node_info)-sizeof(_node_info::flags))!=0){
          node_info[sn]=node;
          if(QMandala::instance()->current==this){
            //save name-by-sn file (cache)
            QDir backup_dir(AppDirs::nodes().filePath("all"));
            if(!backup_dir.mkpath(".")){
              qWarning("%s",tr("Error creating backup path").toUtf8().data());
            }else{
              QString ssn=sn.toHex().toUpper();
              QSettings s(backup_dir.filePath(ssn),QSettings::IniFormat);
              s.beginGroup(ssn);
              s.setValue("name",QString((const char*)node->name));
              s.setValue("version",QString((const char*)node->version));
              s.setValue("hardware",QString((const char*)node->hardware));
              s.setValue("timestamp",QDateTime::currentDateTimeUtc().toString());
              s.endGroup();
            }
            //print report
            QString ver=node->version[0]?QString("v%1").arg((const char*)node->version):QString("<LOADER>");
            QString hw=node->hardware[0]?QString("(%1) ").arg((const char*)node->hardware):QString();
            QString descr=QString("%1 %2").arg(ver).arg(hw);
            qDebug("#%s\t%s",(const char*)node->name,descr.toUtf8().data());
          }
        }
      }break;
      case apc_nstat: {
        if(data_cnt!=(sizeof(_node_name)+sizeof(_node_status)))return; //size error
        if(!nstatTimer.isActive())break;
        _node_name node_name;
        _node_status node_status;
        memcpy(&node_name,packet.srv.data,sizeof(_node_name));
        memcpy(&node_status,packet.srv.data+sizeof(_node_name),sizeof(_node_status));
        //print report
        QString snode;
        snode=QString().sprintf("#[%s]%.2X:%.2X E:%.2X C:%.2X %*u%%",node_name,node_status.can_adr,node_status.can_rxc,node_status.err_cnt,node_status.can_err,2,(100*(uint)node_status.load)/255);
        if(node_status.power.VBAT)
          snode+=QString().sprintf("\t%.1fV %umA",node_status.power.VBAT/1000.0,node_status.power.IBAT);
        if(QMandala::instance()->current==this){
          qDebug("%s",snode.toUtf8().data());
          QByteArray ba((const char*)node_status.dump,sizeof(node_status.dump));
          if(ba!=QByteArray(sizeof(node_status.dump),(char)0)){
            qDebug("#%s",ba.toHex().toUpper().data());
          }
        }
      }break;
      case apc_msg: { //message from autopilot
        QString ns;
        if(QMandala::instance()->size()>1) ns=QString("%1/%2").arg(ident.callsign).arg(node_name(sn));
        else ns=node_name(sn);
        QStringList st=QString(QByteArray((const char*)packet.srv.data,data_cnt)).trimmed().split('\n',QString::SkipEmptyParts);
        foreach(QString s,st){
          s=s.trimmed();
          if(s.isEmpty())continue;
          qDebug("<[%s]%s\n",ns.toUtf8().data(),qApp->translate("msg",s.toUtf8().data()).toUtf8().data());
          FactSystem::instance()->sound(s);
          if(s.contains("error",Qt::CaseInsensitive)) setAlarm(s);
          else if(s.contains("fail",Qt::CaseInsensitive)) setAlarm(s);
          else if(s.contains("timeout",Qt::CaseInsensitive)) setWarning(s);
          else if(s.contains("warning",Qt::CaseInsensitive)) setWarning(s);
        }
      }return;
    }//switch
    emit serviceRequest(ba);
    return;
  }
  //not service
  data_cnt-=bus_packet_size_hdr;
  switch (packet.id) {
    case idx_data:
      if(data_cnt<2){
        qWarning("Mandala: %s",tr("error serial data received").toUtf8().data());
        return;
      }
      emit serialData(packet.data[0],QByteArray((const char*)(packet.data+1),data_cnt-1));
      //dump(packet.data,data_cnt);
      return;
    case idx_xpdr:{ //XPDR tranceiver
      if(data_cnt!=sizeof(IDENT::_xpdr))return;
      IDENT::_xpdr *xpdr=(IDENT::_xpdr*)packet.data;
      if(ident.squawk!=xpdr->squawk)return;
      gps_pos[0]=xpdr->lat;
      gps_pos[1]=xpdr->lon;
      uint dt_a=dataTime.elapsed();
      if(dt_a>50 && dt_a<10000) vspeed=(xpdr->alt-altitude)/(dt_a/1000.0);
      else vspeed=0;
      altitude=xpdr->alt;
      gSpeed=xpdr->gSpeed/100.0;
      course=xpdr->crs*(180.0/32768.0);
      mode=xpdr->mode;
      //substitute some vars
      theta[2]=course;
      emit updated(idx_downstream);
      //qDebug()<<ident.callsign<<ident.squawk<<"squawk";
      setXpdrData(true);
      //save data for telemetry
      if(!dlinkData()){
        QByteArray tba(ba);
        _bus_packet &p=*(_bus_packet*)tba.data();
        p.id=idx_downstream;
        dl_timestamp+=1000;
        rec->record_downlink(tba);
      }
    }return;
  }
  //qDebug()<<ba.toHex();
  //standard extractable var
  if(extract(packet.data,data_cnt,packet.id)) {
    //if((packet.id>=idx_gcu_RSS)||(strchr((const char*)vars_dlink,packet.id)))
    emit updated(packet.id);
    if(packet.id==idx_downstream){
      //qDebug()<<ba.toHex();
      //qDebug()<<theta[0];
      setDlinkData(true);
      QMandala::instance()->setErrcnt(dl_errcnt);
    }
  }else emit data(packet.id,QByteArray((const char*)packet.data,data_cnt));
}
//=============================================================================
bool QMandalaItem::isBroadcast(const QByteArray &sn) const
{
  for(int i=0;i<sn.size();i++)
    if(((const uint8_t*)sn.data())[i]!=0)return false;
  return true;
}
const char * QMandalaItem::node_sn(const QByteArray &sn)
{
  return QByteArray((const char*)sn,sizeof(_node_sn)).toHex().toUpper();
}
const char * QMandalaItem::node_name(const QByteArray &sn)
{
  if(node_info.contains(sn)) return (const char*)node_info.value(sn)->name;
  //try cache lookup
  QString ssn=sn.toHex().toUpper();
  QString fname=AppDirs::nodes().filePath("all/%1").arg(ssn);
  if(!QFile::exists(fname))return "";
  QSettings s(fname,QSettings::IniFormat);
  s.beginGroup(ssn);
  if(s.contains("apc_info")){
    QByteArray ba(QByteArray::fromHex(s.value("apc_info").toString().toUtf8()));
    if(ba.size()==sizeof(_node_info)){
      _node_info *node=new _node_info;
      if(QString((const char*)node->name)!=s.value("name").toString()) return "";
      memcpy(node,ba.data(),ba.size());
      node_info.insert(sn,node);
      return node_name(sn);
    }
  }
  return "";
}
//=============================================================================
void QMandalaItem::req_nodes(void)
{
  //qDebug()<<"req_nodes"<<objectName();
  qDeleteAll(node_info);
  node_info.clear();
  send_srv(apc_search,QByteArray());
}
void QMandalaItem::req_nstat(void)
{
  req_nstat_idx=0;
  if(node_info.uniqueKeys().isEmpty()){
    req_nodes();
    nstatTimer.start(1000);
  }else nstatTimer.start(50);
}
void QMandalaItem::req_nstat_next(void)
{
  if(req_nstat_idx>=node_info.uniqueKeys().size()){
    nstatTimer.stop();
    return;
  }
  send_srv(apc_nstat,node_info.uniqueKeys().at(req_nstat_idx++));
  if(req_nstat_idx>=node_info.uniqueKeys().size()) nstatTimer.start(1000);
  else nstatTimer.start(50);
}
//=============================================================================
//=============================================================================
void QMandalaItem::emitUpdated(uint var_idx)
{
  emit updated(var_idx);
}
//=============================================================================
void QMandalaItem::apcfgUpdated(void)
{
  if(QMandala::instance()->local==this && (!apcfg.value("comment").toString().isEmpty())){
    //strncpy(ident.callsign,apcfg.value("comment").toString().toUtf8().data(),sizeof(ident.callsign));
    if(QMandala::instance()->current==this) QMandala::instance()->setUavName(QString(ident.callsign));
  }
  emit apcfgChanged();
}
//=============================================================================
//=============================================================================
void QMandalaItem::print_report(FILE *stream)
{
  fprintf(stream,"======= %s ================\n",tr("GCU accessible names").toUtf8().data());
  fprintf(stream,"%s\t%s\t%s\t%s\n",tr("TextName").toUtf8().data(),tr("Description").toUtf8().data(),tr("Index").toUtf8().data(),tr("Component").toUtf8().data());
  foreach(QMandalaField *f,fields){
    fprintf(stream,"%s\t%s\t%u\t%u\n",f->name().toUtf8().data(),f->descr().toUtf8().data(),f->varmsk()&0xFF,f->varmsk()>>8);
  }
  fprintf(stream,"\n");
  fprintf(stream,"======= Mandala %s ===================\n",tr("Structure").toUtf8().data());
  fprintf(stream,"%s\t%s\t%s\t%s\n",tr("Name").toUtf8().data(),tr("Description").toUtf8().data(),tr("Index [hex]").toUtf8().data(),tr("Values").toUtf8().data());
  for(uint var_idx=0;var_idx<idx_vars_top;var_idx++){
    uint type;
    void *value_ptr;
    if(!get_ptr(var_idx,&value_ptr,&type))continue;
    const char *var_name;
    const char *var_descr;
    if(!get_text_names(var_idx|0xFF00,&var_name,&var_descr))break;
    const char *vt="";
    switch(type){
      case vt_void:   vt="UNKNOWN";break;
      case vt_byte:   vt="byte";break;
      case vt_uint:   vt="uint";break;
      case vt_flag:   vt="flag";break;
      case vt_enum:   vt="enum";break;
      case vt_float:  vt="float";break;
      case vt_vect:   vt="vect";break;
      case vt_point:  vt="point";break;
      case vt_idx:    vt="index";break;
    }
    fprintf(stream,"%s\t%s\t%s\t%.2X",var_name,vt,var_descr,var_idx);
    //get enums
    const char *name;
    const char *descr;
    switch(type){
      case vt_flag: {
        fprintf(stream,"\t");
        uint m=1;
        while((m<0x0100)&&get_text_names(var_idx|(m<<8),&name,&descr)){
          if(m>1)fprintf(stream,",");
          fprintf(stream,"%s=%u",name,m);
          m<<=1;
        }
      }break;
      case vt_enum: {
        fprintf(stream,"\t");
        uint m=0;
        while(get_text_names(var_idx|(m<<8),&name,&descr)){
          if(m)fprintf(stream,",");
          fprintf(stream,"%s=%u",name,m);
          m++;
        }
      }break;
    }
    fprintf(stream,"\n");
  }
}
//=============================================================================
//=============================================================================
bool QMandalaItem::dlinkData()
{
  return m_dlinkData;
}
void QMandalaItem::setDlinkData(bool v)
{
  if(v){
    dlinkDataTimer.start();
    dataTime.start();
  }
  if(m_dlinkData==v)return;
  m_dlinkData=v;
  emit dlinkDataChanged(v);
  emit aliveChanged(alive());
}
void QMandalaItem::dlinkDataTimeout()
{
  setDlinkData(false);
}
bool QMandalaItem::xpdrData()
{
  return m_xpdrData;
}
void QMandalaItem::setXpdrData(bool v)
{
  if(v){
    xpdrDataTimer.start();
    dataTime.start();
  }
  if(m_xpdrData==v)return;
  m_xpdrData=v;
  emit xpdrDataChanged(v);
  emit aliveChanged(alive());
}
void QMandalaItem::xpdrDataTimeout()
{
  setXpdrData(false);
}
bool QMandalaItem::replayData()
{
  return m_replayData;
}
void QMandalaItem::setReplayData(bool v)
{
  if(v){
    replayDataTimer.start();
    dataTime.start();
  }
  if(m_replayData==v)return;
  m_replayData=v;
  emit replayDataChanged(v);
}
void QMandalaItem::replayDataTimeout()
{
  setReplayData(false);
}
bool QMandalaItem::alive()
{
  return xpdrData()||dlinkData();
}
bool QMandalaItem::active()
{
  return m_active;
}
void QMandalaItem::setActive(bool v)
{
  if(m_active==v)return;
  m_active=v;
  emit activeChanged(v);
}
FlightDataFile * QMandalaItem::recorder()
{
  return rec;
}
QString QMandalaItem::alarm()
{
  return m_alarm;
}
void QMandalaItem::setAlarm(QString v)
{
  m_alarm=v;
  emit alarmChanged(v);
  FactSystem::instance()->sound("error");
}
QString QMandalaItem::warning()
{
  return m_warning;
}
void QMandalaItem::setWarning(QString v)
{
  m_warning=v;
  emit warningChanged(v);
  FactSystem::instance()->sound("warning");
}
//=============================================================================
//=============================================================================
