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
#include <Facts.h>
#include <QtSql>
#include "NodeItem.h"
#include "Nodes.h"
#include "NodeField.h"
#include "PawnScript.h"
#include <node.h>
//=============================================================================
NodeItem::NodeItem(Nodes *parent, const QByteArray &sn)
  : NodeItemData(parent,sn),
    timeout_ms(500),
    nodes(parent),
    group(NULL),
    lastSeenTime(0),
    skipCache(false),
    m_infoValid(false)
{
  qmlRegisterUncreatableType<NodeItem>("GCS.Node", 1, 0, "Node", "Reference only");

  sortNames<<"shiva"<<"nav"<<"ifc"<<"swc"<<"cas"<<"gps"<<"mhx"<<"servo"<<"bldc";

  commands.valid=false;


  //setQmlMenu("nodes/NodeMenuItem.qml");

  connect(this,&NodeItem::versionChanged,this,&NodeItem::updateStats);
  connect(this,&NodeItem::hardwareChanged,this,&NodeItem::updateStats);

  connect(this,&NodeItem::progressChanged,nodes,&Nodes::updateProgress);

  //datalink
  connect(this,&NodeItem::nmtRequest,parent->vehicle->nmtManager,&VehicleNmtManager::request);

  connect(this,&NodeItem::dictValidChanged,this,&NodeItem::validateDict);
  connect(this,&NodeItem::dataValidChanged,this,&NodeItem::validateData);
  connect(this,&NodeItem::infoValidChanged,this,&NodeItem::validateInfo);

  connect(this,&NodeItem::reconfChanged,this,&NodeItem::updateReconf);

  syncTimer.setSingleShot(true);
  connect(&syncTimer,&QTimer::timeout,this,&NodeItem::sync);

  Vehicles::instance()->vdb->nodeInfoRead(this);

  sync();
}
//=============================================================================
void NodeItem::validateDict()
{
  if(group){
    bool ok=true;
    foreach (FactTree *i, group->childItems()) {
      if(static_cast<NodeItem*>(i)->dictValid())continue;
      ok=false;
      break;
    }
    group->setDictValid(ok,false);
  }
  if(!dictValid()){
    bool ok=true;
    foreach (NodeField *f, allFields) {
      if(f->dictValid())continue;
      ok=false;
      break;
    }
    if(!ok)return;
    //all fields downloaded and valid
    if(commands.valid){
      setDictValid(true);
      //qDebug()<<node->path();
    }
    return;
  }
  foreach (NodeField *f, allFields) {
    f->createSubFields();
  }
  groupFields();
  setDictValid(true); //recursive update valid for children
  Vehicles::instance()->vdb->nodeDictWrite(this);
  //qDebug()<<"Node valid"<<path();
  FactSystem::instance()->jsSync(this);
}
void NodeItem::validateData()
{
  if(group){
    bool ok=true;
    foreach (FactTree *i, group->childItems()) {
      if(static_cast<NodeItem*>(i)->dataValid())continue;
      ok=false;
      break;
    }
    group->setDataValid(ok,false);
  }
  if(!dataValid())return;
  setProgress(0);
  if(reconf()){
    Vehicles::instance()->vdb->nodeDataRestore(this);
  }else{
    Vehicles::instance()->vdb->nodeDataWrite(this);
  }
  //qDebug()<<"Node dataValid"<<path();
}
void NodeItem::validateInfo()
{
  if(!infoValid())return;
  groupNodes();
  Vehicles::instance()->vdb->nodeInfoWrite(this);
  //FactSystem::instance()->jsSync(this);
  //qDebug()<<"Node infoValid"<<path();
}
void NodeItem::updateReconf()
{
  if(!reconf()) setDataValid(false);
}
void NodeItem::sync()
{
  request(apc_info,QByteArray(),timeout_ms,true);
}
void NodeItem::syncLater(int timeout)
{
  if(syncTimer.isActive() && timeout>syncTimer.interval())
    timeout=syncTimer.interval();
  syncTimer.start(timeout);
  //qDebug()<<timeout;
}
//=============================================================================
void NodeItem::confWritten()
{
  if(reconf()){
    syncLater(3000);
  }else{
    Vehicles::instance()->vdb->nodeDataWrite(this);
  }
}
//=============================================================================
void NodeItem::updateStats()
{
  setDescr(QString("%1 %2 %3").arg(m_hardware).arg(m_version).arg(QString(sn.toHex().toUpper())));
}
//=============================================================================
void NodeItem::nstat()
{
  request(apc_nstat,QByteArray(),0,false);
}
//=============================================================================
void NodeItem::upload()
{
  if(!(dictValid()&&dataValid()))return;
  if(!modified())return;
  saveTelemetryUploadEvent();
  foreach(NodeField *f,allFields){
    if(!f->modified())continue;
    if(f->script) f->script->upload();
    else request(apc_conf_write,QByteArray().append((unsigned char)f->id).append(f->packValue()),1000);
  }
  request(apc_conf_write,QByteArray().append((unsigned char)0xFF),1000);
}
//=============================================================================
void NodeItem::saveTelemetryUploadEvent()
{
  foreach(NodeField *f,allFields){
    if(!f->modified())continue;
    if(f->size()){
      foreach (FactTree *i, f->childItems()) {
        NodeField *fx=static_cast<NodeField*>(i);
        if(!fx->modified())continue;
        nodes->vehicle->f_recorder->recordEvent("conf",QString("%1/%2=%3").arg(title()).arg(fx->name()).arg(fx->text()),true,sn);
      }
    }else{
      nodes->vehicle->f_recorder->recordEvent("conf",QString("%1/%2=%3").arg(title()).arg(f->name()).arg(f->text()),true,sn);
    }
  }
}
//=============================================================================
bool NodeItem::unpackService(uint ncmd, const QByteArray &ba)
{
  switch(ncmd){
    case apc_search:
      sync();
    return true;
    case apc_info: {
      //fill available nodes
      if((uint)ba.size()<sizeof(_node_name))break;
      _node_info ninfo;
      memset(&ninfo,0,sizeof(_node_info));
      if((uint)ba.size()>sizeof(_node_info)){
        memcpy(&ninfo,ba.data(),sizeof(_node_info));
      }else{
        memcpy(&ninfo,ba.data(),ba.size());
      }
      ninfo.name[sizeof(_node_name)-1]=0;
      //populate info
      lastSeenTime=QDateTime::currentDateTime().toTime_t();
      setName((const char*)ninfo.name);
      setTitle((const char*)ninfo.name);
      setVersion(QString((const char*)ninfo.version));
      setHardware(QString((const char*)ninfo.hardware));
      setReconf(ninfo.flags.conf_reset);
      setFwSupport(ninfo.flags.loader_support);
      setFwUpdating(ninfo.flags.in_loader);
      setAddressing(ninfo.flags.addressing);
      setRebooting(ninfo.flags.reboot);
      setBusy(ninfo.flags.busy);
      setFailure(false);
      setInfoValid(true);
      //Vehicles::instance()->vdb->nodeInfoWrite(this);
      request(apc_conf_inf,QByteArray(),timeout_ms,true);
    }return true;
    case apc_nstat: {
      if(ba.size()!=(sizeof(_node_name)+sizeof(_node_status)))break;
      _node_status nstatus;
      memcpy(&nstatus,ba.data()+sizeof(_node_name),sizeof(_node_status));
      setVbat(nstatus.power.VBAT/1000.0);
      setIbat(nstatus.power.IBAT/1000.0);
      setErrCnt(nstatus.err_cnt);
      setCanRxc(nstatus.can_rxc);
      setCanAdr(nstatus.can_adr);
      setCanErr(nstatus.can_err);
      setCpuLoad((uint)nstatus.load*100/255);
      //print report
      QString snode;
      //snode=QString().sprintf("#[%s]%.2X:%.2X E:%.2X C:%.2X %*u%%",node_name,node_status.can_adr,node_status.can_rxc,node_status.err_cnt,node_status.can_err,2,(100*(uint)node_status.load)/255);
      snode=QString("#[%1]%2:%3 E:%4 C:%5 %6%").arg(title()).arg(canAdr(),2,16,QChar('0')).arg(canRxc(),2,16,QChar('0')).arg(errCnt(),2,16,QChar('0')).arg(canErr(),2,16,QChar('0')).arg(cpuLoad()).toUpper();
      if(vbat())
        snode+=QString("\t%1V %2mA").arg(vbat(),0,'g',1).arg((int)(ibat()*1000.0));
      if(nodes->vehicle==Vehicles::instance()->current()){
        qDebug("%s",snode.toUtf8().data());
        QByteArray ba((const char*)nstatus.dump,sizeof(nstatus.dump));
        if(ba!=QByteArray(sizeof(nstatus.dump),(char)0)){
          qDebug("#%s",ba.toHex().toUpper().data());
        }
      }

    }return true;
    case apc_msg: { //message from autopilot
      const QString &s=QString(ba);
      message(s);
      if(reconf() && (!modified()) && s==QString("%1 initialized").arg(title())){
        syncLater(5000);
      }
    }return true;
    case apc_conf_inf: {
      if(ba.size()!=sizeof(_conf_inf))break;
      QString hash=ba.toHex().toUpper();
      _conf_inf conf_inf;
      memcpy(&conf_inf,ba.data(),sizeof(_conf_inf));
      if(conf_inf.cnt!=allFields.size() || hash!=conf_hash){
        allFields.clear();
        removeAll();
        conf_hash=hash;
        setDictValid(false);
        for(quint16 id=0;id<conf_inf.cnt;id++){
          allFields.append(new NodeField(this,id));
        }
        //qDebug()<<"fields created"<<conf_inf.cnt;
      }
      Vehicles::instance()->vdb->vehicleNodesUpdate(nodes->vehicle);
      if(!dictValid()){
        if(skipCache){
          skipCache=false;
        }else{
          Vehicles::instance()->vdb->nodeDictRead(this);
        }
      }
      if(!commands.valid){
        request(apc_conf_cmds,QByteArray(),timeout_ms,false);
      }else requestConf();
    }return true;
    case apc_conf_cmds: {
      if((!commands.valid)||ba.size()>0){
        commands.cmd.clear();
        commands.name.clear();
        commands.descr.clear();
        const char *str=(const char*)ba.data();
        int cnt=ba.size();
        int sz;
        while(cnt>0){
          uint cmd=(unsigned char)*str++;
          cnt--;
          sz=strlen(str)+1;
          if(sz>cnt)break;
          QString name(QByteArray(str,sz-1));
          str+=sz;
          cnt-=sz;
          sz=strlen(str)+1;
          if(sz>cnt)break;
          QString descr(QByteArray(str,sz-1));
          str+=sz;
          cnt-=sz;
          commands.cmd.append(cmd);
          commands.name.append(name);
          commands.descr.append(descr);
        }
        if(cnt!=0){
          qWarning("Error node_conf commands received (cnt:%u)",cnt);
          commands.cmd.clear();
          commands.name.clear();
          commands.descr.clear();
        }else{
          commands.valid=true;
          requestConf();
          if(!dictValid()){
            validateDict();
            /*if(dictValid()){
              Vehicles::instance()->vdb->nodeDictWrite(this);
            }*/
          }
          //qDebug()<<commands.name;
        }
      }
    }return true;
    case apc_conf_read:
    case apc_conf_write:
      if(!dictValid())return true;
      if(ba.isEmpty()){
        //qDebug()<<"node conf written";
        if(!modified())confWritten();
      }
    case apc_conf_dsc:
    {
      if(ba.size()<1)break;
      NodeField *field=allFields.value((unsigned char)ba.at(0),NULL);
      if(!field)return true;
      if(field->unpackService(ncmd,ba.mid(1))){
        updateProgress();
        if(ncmd==apc_conf_dsc)requestConf();
        return true;
      }
    }break;
    case apc_script_file:
    case apc_script_read:
    case apc_script_write: {
      if(!dictValid())return true;
      bool rv=false;
      foreach (NodeField *f, allFields) {
        if(f->unpackService(ncmd,ba)) rv=true;
      }
      return rv;
    }break;
  }
  //error

  return true;
}
//=============================================================================
void NodeItem::updateProgress()
{
  if(!(dictValid()&&dataValid())){
    int ncnt=0;
    foreach (NodeField *f, allFields) {
      if(f->dictValid())ncnt+=7;
      if(f->dataValid())ncnt+=3;
    }
    setProgress(ncnt?(ncnt*100)/allFields.size()/10:0);
  }
}
//=============================================================================
void NodeItem::requestConf()
{
  //sync all conf if needed
  if(!dictValid()){
    foreach (NodeField *f, allFields) {
      if(f->dictValid())continue;
      request(apc_conf_dsc,QByteArray().append((unsigned char)f->id),timeout_ms,false);
      break; //only once
    }
  }else if(!dataValid()){
    foreach (NodeField *f, allFields) {
      if(f->dataValid())continue;
      request(apc_conf_read,QByteArray().append((unsigned char)f->id),timeout_ms,false);
      break; //only once
    }
  }
}
//=============================================================================
void NodeItem::message(QString msg)
{
  QString ns;
  if(Vehicles::instance()->f_list->size()>0) ns=QString("%1/%2").arg(nodes->vehicle->f_callsign->text()).arg(title());
  else ns=title();
  QStringList st=msg.trimmed().split('\n',QString::SkipEmptyParts);
  foreach(QString s,st){
    s=s.trimmed();
    if(s.isEmpty())continue;
    qDebug("<[%s]%s\n",ns.toUtf8().data(),qApp->translate("msg",s.toUtf8().data()).toUtf8().data());
    nodes->vehicle->f_recorder->recordEvent("msg",QString("[%1]%2").arg(ns).arg(s),false,sn);
    FactSystem::instance()->sound(s);
    if(s.contains("error",Qt::CaseInsensitive)) nodes->vehicle->f_warnings->error(s);
    else if(s.contains("fail",Qt::CaseInsensitive)) nodes->vehicle->f_warnings->error(s);
    else if(s.contains("timeout",Qt::CaseInsensitive)) nodes->vehicle->f_warnings->warning(s);
    else if(s.contains("warning",Qt::CaseInsensitive)) nodes->vehicle->f_warnings->warning(s);
  }
}
//=============================================================================
void NodeItem::groupFields(void)
{
  foreach (FactTree *i, allFields) {
    NodeField *f=static_cast<NodeField*>(i);
    //grouping
    Fact *groupItem=NULL;
    Fact *groupParent=this;
    foreach(QString group,f->groups){
      groupItem=NULL;
      QString gname=group.toLower();
      foreach(FactTree *i,groupParent->childItems()){
        Fact *f=static_cast<Fact*>(i);
        if(!(f->treeItemType()==GroupItem && f->title().toLower()==gname))continue;
        groupItem=f;
        break;
      }
      if(!groupItem)
        groupItem=new NodeFieldBase(groupParent,gname,group,"",GroupItem,NoData);
      groupParent=groupItem;
      if(f->parentItem()) f->parentItem()->removeItem(f,false);
      groupItem->addItem(f);
      f->setSection("");
    }
    if(!f->parentItem()) addItem(f);
    //connect modified signals
    /*for(FactTree *i=f;i!=parentItem();i=i->parentItem()){
      connect(static_cast<Fact*>(i),&Fact::modifiedChanged,static_cast<Fact*>(i->parentItem()),&Fact::modifiedChanged);
    }*/
    //hide grouped arrays (gpio, controls etc)
    if(groupItem && groupItem->size()>=2){
      bool bArray=false;
      uint cnt=0;
      foreach(FactTree *i,groupItem->childItems()){
        NodeField *f=static_cast<NodeField*>(i);
        bArray=false;
        if(cnt<2 && f->array()<=1)break;
        if((int)f->array()!=f->size())break;
        //if(!array_sz)array_sz=field_item->array;
        //if(field_item->array!=array_sz)break;
        cnt++;
        bArray=true;
      }
      if(bArray){
        //qDebug()<<cnt<<groupItem->path();
        foreach(FactTree *i,groupItem->childItems()){
          NodeField *f=static_cast<NodeField*>(i);
          f->setVisible(false);
        }
        connect(static_cast<Fact*>(groupItem->child(0)),&Fact::statusChanged,this,[=](){groupItem->setStatus(static_cast<Fact*>(groupItem->child(0))->status());});
        groupItem->setStatus(static_cast<Fact*>(groupItem->child(0))->status());
      }
    }
  }//foreach field
}
//=============================================================================
void NodeItem::groupNodes(void)
{
  //check node grouping
  if(group)return;
  NodeItemBase *ngroup=NULL;
  //find same names with same parent
  QList<NodeItem*>nlist;
  QString stitle=title();
  if(stitle.endsWith(".shiva"))stitle.remove(0,stitle.lastIndexOf('.')+1);
  QString gname=stitle;
  QStringList names;
  names.append(stitle);
  if(stitle=="bldc"){
    gname="servo";
    names.append(gname);
  }
  foreach(NodeItem *i,nodes->snMap.values()){
    QString s=i->title();
    if(s.endsWith(".shiva"))s.remove(0,s.lastIndexOf('.')+1);
    if(names.contains(s))
      nlist.append(i);
  }
  foreach (NodeItemBase *g, nodes->nGroups) {
    if(g->size()>0 && g->title()==gname.toUpper()){
      ngroup=g;
      break;
    }
  }

  if(ngroup==NULL && nlist.size()<2){
    //nodes->f_list->addItem(this);
    return;
  }
  //qDebug()<<"-append-";

  if(ngroup)group=ngroup;
  else {
    group=new NodeItemBase(nodes,gname,gname.toUpper());
    group->setSection(section());
    nodes->nGroups.append(group);
    //qDebug()<<"grp: "<<gname;
  }

  foreach(NodeItem *i,nlist){
    if(i->parentItem()==group)continue;
    nodes->removeItem(i,false);
    group->addItem(i);
    i->group=group;
    i->setName(i->name()); //update unique name
    //qDebug()<<gname<<"<<"<<i->name;
    //if(node->name.contains("shiva")) qDebug()<<node->name<<nlist.size()<<(group?group->name:"");
  }
  //update group descr
  QStringList gNames,gHW;
  foreach(FactTree *i,group->childItems()){
    NodeItem *n=static_cast<NodeItem*>(i);
    if(!gNames.contains(n->title()))gNames.append(n->title());
    if(!gHW.contains(n->hardware()))gHW.append(n->hardware());
  }
  QStringList sdescr;
  if(!gNames.isEmpty())sdescr.append(gNames.join(','));
  if(!gHW.isEmpty())sdescr.append("("+gHW.join(',')+")");
  if(!sdescr.isEmpty())group->setDescr(sdescr.join(' '));
  group->setStatus(QString("[%1]").arg(group->size()));
}
//=============================================================================
void NodeItem::request(uint cmd,const QByteArray &data,uint timeout_ms,bool highprio)
{
  emit nmtRequest(cmd,sn,data,timeout_ms,highprio);
}
//=============================================================================
//=============================================================================
QString NodeItem::version() const
{
  return m_version;
}
void NodeItem::setVersion(const QString &v)
{
  if(m_version==v)return;
  m_version=v;
  emit versionChanged();
}
QString NodeItem::hardware() const
{
  return m_hardware;
}
void NodeItem::setHardware(const QString &v)
{
  if(m_hardware==v)return;
  m_hardware=v;
  emit hardwareChanged();
}
bool NodeItem::infoValid() const
{
  return m_infoValid;
}
void NodeItem::setInfoValid(const bool &v)
{
  if(m_infoValid==v)return;
  m_infoValid=v;
  emit infoValidChanged();
}
//=============================================================================
//=============================================================================
QVariant NodeItem::data(int col, int role) const
{
  if(dictValid() && dataValid()){
    switch(role){
      case Qt::ForegroundRole:
        //if(isUpgrading())return QColor(Qt::white);
        //return col==FACT_MODEL_COLUMN_NAME?QColor(Qt::darkYellow):QColor(Qt::darkGray);
        //if(!statsShowTimer.isActive()) return isUpgradable()?QColor(Qt::red).lighter():Qt::darkGray;
        //nstats
        //return statsWarn?QColor(Qt::yellow):QColor(Qt::green);
      break;
      case Qt::BackgroundRole:
        //if(isUpgrading())return QColor(0x20,0x00,0x00);
        //if(isUpgradePending())return QColor(0x40,0x00,0x00);
        if(reconf())return QColor(Qt::darkGray).darker(200);
        return QColor(0x20,0x40,0x60);
    }
  }
  return NodeItemData::data(col,role);
}
//=============================================================================
void NodeItem::hashData(QCryptographicHash *h) const
{
  Fact::hashData(h);
  h->addData(version().toUtf8());
  h->addData(hardware().toUtf8());
  h->addData(conf_hash.toUtf8());
  h->addData(QString::number(fwSupport()).toUtf8());
  for(int i=0;i<commands.cmd.size();++i){
    h->addData(QString::number(commands.cmd.at(i)).toUtf8());
    h->addData(commands.name.at(i).toUtf8());
    h->addData(commands.descr.at(i).toUtf8());
  }
}
//=============================================================================
//=============================================================================
void NodeItem::clearCommands()
{
  commands.cmd.clear();
  commands.name.clear();
  commands.descr.clear();
  commands.valid=false;
}
//=============================================================================
void NodeItem::cmdexec(int cmd_idx)
{
  if(cmd_idx>=commands.cmd.size()){
    qWarning("%s",tr("Can't send command (unknown command)").toUtf8().data());
    nodes->request();
    return;
  }
  QString cmd_name=commands.name.at(cmd_idx);
  /*if(cmd_name=="bb_read"){
    //qDebug("cmd: %s",cmd_name.toUtf8().data());
    if(blackboxDownload){
      blackboxDownload->show();
    }else{
      blackboxDownload=new BlackboxDownload(this);
      connect(blackboxDownload,SIGNAL(finished()),this,SLOT(blackboxDownloadFinished()));
      connect(blackboxDownload,SIGNAL(request(uint,QByteArray,QByteArray,uint)),this,SIGNAL(request(uint,QByteArray,QByteArray,uint)));
      connect(blackboxDownload,SIGNAL(started()),&model->requestManager,SLOT(enableTemporary()));
      //connect(blackboxDownload,SIGNAL(finished()),&model->requestManager,SLOT(stop()));
      blackboxDownload->show();
    }
    return;
  }*/

  //qDebug("cmd (%u)",cmd);
  request(commands.cmd.at(cmd_idx),QByteArray(),500);
  if(commands.cmd.at(cmd_idx)==apc_reconf || commands.name.at(cmd_idx).startsWith("conf")){
    setDataValid(false);
    syncLater();
  }
}
//=============================================================================
QHash<QString,NodeField*> NodeItem::allFieldsDataMap()
{
  QHash<QString,NodeField*> map;
  for(int i=0;i<allFields.size();++i){
    NodeField *f=allFields.at(i);
    //expand complex numbers
    bool bComplex=f->size()>0;
    if(bComplex){
      for(int i2=0;i2<f->size();++i2){
        NodeField *f2=static_cast<NodeField*>(f->childItems().at(i2));
        map.insert(f->name()+"/"+f2->name(),f2);
      }
    }else map.insert(f->name(),f);
  }
  return map;
}
//=============================================================================
//=============================================================================
