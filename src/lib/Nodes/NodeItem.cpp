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
#include <node.h>
//=============================================================================
NodeItem::NodeItem(Nodes *parent, const QByteArray &sn)
  : NodeData(parent->f_list,sn),
    timeout_ms(500),
    nodes(parent),
    m_valid(false),
    m_dataValid(false)
{
  qmlRegisterUncreatableType<NodeItem>("GCS.Node", 1, 0, "Node", "Reference only");

  sortNames<<"shiva"<<"nav"<<"ifc"<<"swc"<<"cas"<<"gps"<<"mhx"<<"servo"<<"bldc";

  commands.valid=false;

  setSection(tr("Nodes list"));

  //setQmlMenu("nodes/NodeMenuItem.qml");

  connect(this,&NodeItem::versionChanged,this,&NodeItem::updateStats);
  connect(this,&NodeItem::hardwareChanged,this,&NodeItem::updateStats);

  connect(this,&NodeItem::progressChanged,nodes,&Nodes::updateProgress);

  //datalink
  connect(this,&NodeItem::nmtRequest,parent->vehicle->nmtManager,&VehicleNmtManager::request);

  request(apc_info,QByteArray(),timeout_ms,true);

  dbRegister(apc_search);
}
//=============================================================================
void NodeItem::dbRegister(int state)
{
  QSqlQuery query(*FactSystem::db());
  while(FactSystem::db()->isOpen()){
    switch(state){
      case apc_search:
        //register node sn
        query.prepare("INSERT INTO Nodes(sn, date) VALUES(?, ?)");
        query.addBindValue(sn.toHex().toUpper());
        query.addBindValue(QDateTime::currentDateTime().toTime_t());
        query.exec();
        FactSystem::db()->commit();
      return;
      case apc_info:
        //modify node info
        query.prepare("UPDATE Nodes SET date=?, name=?, version=?, hardware=? WHERE sn=?");
        query.addBindValue(QDateTime::currentDateTime().toTime_t());
        query.addBindValue(title());
        query.addBindValue(version());
        query.addBindValue(hardware());
        query.addBindValue(sn.toHex().toUpper());
        query.exec();
      break;
      case apc_conf_inf:
        //modify node conf hash
        query.prepare("UPDATE Nodes SET date=?, hash=?, fcnt=? WHERE sn=?");
        query.addBindValue(QDateTime::currentDateTime().toTime_t());
        query.addBindValue(conf_hash);
        query.addBindValue(allFields.size());
        query.addBindValue(sn.toHex().toUpper());
        query.exec();
      break;
      case apc_conf_dsc:
        //save all fields structure in NodeFields table
        query.prepare("DELETE FROM NodesDict WHERE sn=? AND hash=?");
        query.addBindValue(sn.toHex().toUpper());
        query.addBindValue(conf_hash);
        query.exec();
        uint t=QDateTime::currentDateTime().toTime_t();
        foreach (NodeField *f, allFields) {
          query.prepare(
            "INSERT INTO NodesDict("
            "sn, hash, date, id, name, title, descr, ftype, array, opts, sect"
            ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
          query.addBindValue(sn.toHex().toUpper());
          query.addBindValue(conf_hash);
          query.addBindValue(t);
          query.addBindValue(f->id);
          query.addBindValue(f->name());
          query.addBindValue(f->title());
          query.addBindValue(f->descr());
          query.addBindValue(f->ftypeString());
          query.addBindValue(f->array());
          query.addBindValue(f->enumStrings().join(','));
          query.addBindValue(f->path());
          if(!query.exec())break;
        }
      break;
    }
    if(query.lastError().type()!=QSqlError::NoError)break;
    FactSystem::db()->commit();
    return;
  }
  qWarning() << "NodeItem SQL error:" << query.lastError().text();
  qWarning() << query.executedQuery();
}
//=============================================================================
void NodeItem::updateStats()
{
  setDescr(QString("%1 %2\t%3").arg(m_hardware).arg(m_version).arg(QString(sn.toHex().toUpper())));
}
//=============================================================================
void NodeItem::nstat()
{
  request(apc_nstat,QByteArray(),0,false);
}
//=============================================================================
void NodeItem::upload()
{
  if(!modified())return;
  foreach(NodeField *f,allFields){
    if(!f->modified())continue;
    request(apc_conf_write,QByteArray().append((unsigned char)f->id).append(f->packValue()),1000);
  }
  request(apc_conf_write,QByteArray().append((unsigned char)0xFF),1000);
}
//=============================================================================
bool NodeItem::unpackService(uint ncmd, const QByteArray &ba)
{
  switch(ncmd){
    case apc_search:
      request(apc_info,QByteArray(),timeout_ms,true);
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
      request(apc_conf_inf,QByteArray(),timeout_ms,true);
      FactSystem::instance()->jsSync(this);
      dbRegister(apc_info);
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
      message(QString(ba));
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
        for(quint16 id=0;id<conf_inf.cnt;id++){
          allFields.append(new NodeField(this,id));
        }
        //qDebug()<<"fields created"<<conf_inf.cnt;
      }
      dbRegister(apc_conf_inf);
      if(!commands.valid){
        request(apc_conf_cmds,QByteArray(),timeout_ms,false);
      }else requestConfDsc();
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
          requestConfDsc();
          //qDebug()<<commands.name;
        }
      }
    }return true;
    case apc_conf_dsc:
    case apc_conf_read:
    case apc_conf_write:
    {
      if(ba.size()<1)break;
      NodeField *field=allFields.value((unsigned char)ba.at(0),NULL);
      if(!field)return true;
      if(field->unpackService(ncmd,ba.mid(1))){
        if(!(valid()&&dataValid())){
          int ncnt=0;
          foreach (NodeField *f, allFields) {
            if(f->valid())ncnt+=7;
            if(f->dataValid())ncnt+=3;
          }
          setProgress(ncnt?(ncnt*100)/allFields.size()/10:0);
        }else setProgress(0);
        return true;
      }
    }break;
  }
  //error

  return true;
}
//=============================================================================
void NodeItem::requestConfDsc()
{
  //sync all conf if needed
  if(!(valid() && dataValid())){
    foreach (NodeField *f, allFields) {
      if(!f->valid()) request(apc_conf_dsc,QByteArray().append((unsigned char)f->id),timeout_ms,false);
      //else if(!f->dataValid()) request(apc_conf_read,QByteArray().append((unsigned char)f->id),timeout_ms,false);
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
    while(f->descr().contains(':')){
      QString group=f->descr().left(f->descr().indexOf(':'));
      f->setDescr(f->descr().remove(0,f->descr().indexOf(':')+1).trimmed());
      groupItem=NULL;
      QString gname=group.toLower();
      foreach(FactTree *i,groupParent->childItems()){
        if(!(i->treeItemType()==GroupItem && i->name()==gname))continue;
        groupItem=static_cast<Fact*>(i);
        break;
      }
      if(!groupItem)
        groupItem=new Fact(groupParent,gname,group,"",GroupItem,NoData);
      groupParent=groupItem;
      if(f->title().contains('_') && f->title().left(f->title().indexOf('_'))==group)
        f->setTitle(f->title().remove(0,f->title().indexOf('_')+1));
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
          //f->setVisible(false);
        }
        connect(static_cast<Fact*>(groupItem->child(0)),&Fact::statusChanged,[=](){groupItem->setStatus(static_cast<Fact*>(groupItem->child(0))->status());});
        groupItem->setStatus(static_cast<Fact*>(groupItem->child(0))->status());
      }
    }
  }//foreach field
  FactSystem::instance()->jsSync(this);
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
bool NodeItem::valid() const
{
  return m_valid;
}
void NodeItem::setValid(const bool &v)
{
  if(m_valid==v)return;
  m_valid=v;
  emit validChanged();
}
bool NodeItem::dataValid() const
{
  return m_dataValid;
}
void NodeItem::setDataValid(const bool &v)
{
  if(m_dataValid==v)return;
  m_dataValid=v;
  emit dataValidChanged();
}
//=============================================================================
//=============================================================================
QVariant NodeItem::data(int col, int role) const
{
  switch(role){
    case Qt::ForegroundRole:
      //if(isUpgrading())return QColor(Qt::white);
      if(!valid())return col==FACT_MODEL_COLUMN_NAME?QVariant():QColor(Qt::darkGray);
      if(col==FACT_MODEL_COLUMN_NAME){
        if(modified())return QColor(Qt::red).lighter();
        return QVariant();
      }
      if(col==FACT_MODEL_COLUMN_VALUE)return QColor(Qt::yellow);//QVariant();
      //descr col
      return QColor(Qt::darkGray);
      //if(!statsShowTimer.isActive()) return isUpgradable()?QColor(Qt::red).lighter():Qt::darkGray;
      //nstats
      //return statsWarn?QColor(Qt::yellow):QColor(Qt::green);
    break;
    case Qt::BackgroundRole:
      //if(isUpgrading())return QColor(0x20,0x00,0x00);
      //if(isUpgradePending())return QColor(0x40,0x00,0x00);
      //if(!inf_valid)return QVariant();//QColor(Qt::red).lighter();
      if(!valid())return QColor(50,50,0);
      if(reconf())return QColor(Qt::darkGray).darker(200);
    return QColor(Qt::darkCyan).darker(200);//QColor(0x20,0x40,0x40);//isModified()?QColor(0x40,0x20,0x20):
  }
  return Fact::data(col,role);
}
//=============================================================================
bool NodeItem::lessThan(Fact *rightFact) const
{
  //try to sort by sortNames
  QString sleft=title();
  if(sleft.contains('.'))sleft=sleft.remove(0,sleft.indexOf('.')+1).trimmed();
  QString sright=rightFact->title();
  if(sright.contains('.'))sright=sright.remove(0,sright.indexOf('.')+1).trimmed();
  if(sortNames.contains(sleft)){
    if(sortNames.contains(sright)){
      int ileft=sortNames.indexOf(sleft);
      int iright=sortNames.indexOf(sright);
      if(ileft!=iright) return ileft<iright;
    }else return true;
  }else if(sortNames.contains(sright)) return false;

  //compare names
  int ncmp=QString::localeAwareCompare(title(),rightFact->title());
  if(ncmp!=0)return ncmp<0;
  //try to sort by comment same names
  ncmp=QString::localeAwareCompare(text(), rightFact->text());
  if(ncmp==0){
    //try to sort by sn same names
    ncmp=QString::localeAwareCompare(QString(sn.toHex()), QString(static_cast<NodeItem*>(rightFact)->sn.toHex()));
  }
  if(ncmp==0) return Fact::lessThan(rightFact);
  return ncmp<0;
}
//=============================================================================
void NodeItem::hashData(QCryptographicHash *h) const
{
  Fact::hashData(h);
  h->addData(version().toUtf8());
  h->addData(hardware().toUtf8());
  h->addData(conf_hash.toUtf8());
  h->addData(QString::number(fwSupport()).toUtf8());
}
//=============================================================================
void NodeItem::saveToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("node"));
  dom.toElement().setAttribute("sn",QString(sn.toHex().toUpper()));
  dom.toElement().setAttribute("name",title());

  QDomNode e=dom.appendChild(doc.createElement("info"));
  e.appendChild(doc.createElement("version")).appendChild(doc.createTextNode(version()));
  e.appendChild(doc.createElement("hardware")).appendChild(doc.createTextNode(hardware()));

  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(hash().toHex().toUpper()));
  dom.appendChild(doc.createElement("timestamp")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));

  /*if(commands.cmd.size()){
    for(int i=0;i<commands.cmd.size();i++){
      QDomNode e=dom.appendChild(doc.createElement("command"));
      e.toElement().setAttribute("cmd",QString::number(commands.cmd.at(i)));
      e.appendChild(doc.createElement("name")).appendChild(doc.createTextNode(commands.name.at(i)));
      e.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(commands.descr.at(i)));
    }
  }*/

  e=dom.appendChild(doc.createElement("fields"));
  e.toElement().setAttribute("cnt",QString::number(allFields.size()));
  e.toElement().setAttribute("hash",conf_hash);

  QDomNode domf=e;
  foreach(NodeField *f,allFields){
    //f->saveToXml(e);
    QDomNode dom=domf.appendChild(doc.createElement("field"));
    dom.toElement().setAttribute("idx",QString::number(f->id));
    dom.toElement().setAttribute("name",f->conf_name);

    QDomNode e=dom.appendChild(doc.createElement("struct"));
    e.appendChild(doc.createElement("type")).appendChild(doc.createTextNode(f->ftypeString()));
    if(f->conf_descr.size())e.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(f->conf_descr));
    if(f->enumStrings().size() && f->ftype!=ft_varmsk)e.appendChild(doc.createElement("opts")).appendChild(doc.createTextNode(f->enumStrings().join(',')));
    //value
    if(f->size()){
      foreach(FactTree *i,f->childItems()) {
        Fact *subf=static_cast<Fact*>(i);
        QDomNode e=dom.appendChild(doc.createElement("value"));
        e.toElement().setAttribute("idx",QString::number(subf->num()));
        e.toElement().setAttribute("name",subf->title());
        e.appendChild(doc.createTextNode(subf->text()));
      }
    }else{
      dom.appendChild(doc.createElement("value")).appendChild(doc.createTextNode(f->text()));
    }
  }
}
//=============================================================================
