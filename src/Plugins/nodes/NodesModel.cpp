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
#include <QDomDocument>
#include "NodesModel.h"
#include "QMandala.h"
#include "QMandala.h"
#include "ValueEditor.h"
#include "ValueEditorArray.h"
#include "ValueEditorScript.h"
#include "ValueEditorNgrp.h"
#include "AppDirs.h"
#include "FactSystem.h"
QAction *NodesModel::aUpload=NULL;
//=============================================================================
NodesModel::NodesModel(QMandalaItem *m, QObject *parent)
 : QAbstractItemModel(parent),
   mvar(m),
   requestManager(this)
{
  setObjectName("conf");

  mandala=qApp->property("Mandala").value<QMandala*>();

  //init backup directory
  backup_dir_ok=false;
  QString callsign(m->ident.callsign);
  if(callsign.size()){
    backup_dir.setPath(AppDirs::nodes().path());
    QString spath=QString("vehicle/%1").arg(callsign);
    if(!(backup_dir.mkpath(spath)&&backup_dir.cd(spath))){
      qWarning("%s",tr("Can't create vehicle backup path").toUtf8().data());
    }else{
      backup_dir.setNameFilters(QStringList()<<"*.nodes");
      backup_dir.setSorting(QDir::Name);
      backup_dir_ok=true;
      backup_sidx=new QSettings(backup_dir.filePath("index"),QSettings::IniFormat);
      connect(this,SIGNAL(done()),this,SLOT(saveVehicleBackup()),Qt::QueuedConnection);
    }
  }

  root = new NodesItem();

  connect(mvar,SIGNAL(serviceRequest(QByteArray)),this,SLOT(service(QByteArray)));
  connect(mvar,SIGNAL(updated(uint)),this,SLOT(mandala_updated(uint)));

  connect(mandala->local->rec,SIGNAL(replay_progress(uint)),this,SLOT(replay_progress(uint)));

  connect(&requestManager,SIGNAL(finished()),this,SLOT(requestManagerFinished()));

  nodes_found_cnt=0;
  requestActive=false;

  syncTimer.setSingleShot(true);
  syncTimer.setInterval(500);
  //connect(&syncTimer,SIGNAL(timeout()),this,SLOT(sync()));

  QScriptValue mobj=mvar->engine.newQObject(this,QScriptEngine::QtOwnership,QScriptEngine::ExcludeSuperClassMethods);
  mvar->engine.globalObject().setProperty(objectName(),mobj);
}
NodesModel::~NodesModel()
{
  clear();
  delete root;
  delete backup_sidx;
}
//=============================================================================
void NodesModel::mandala_updated(uint var_idx)
{
  if(var_idx!=idx_downstream || mandala->size()>1)return;
  if((!syncTimer.isActive()) && nodes.isEmpty())
    syncTimer.start();
}
//=============================================================================
void NodesModel::clear(void)
{
  if(isUpgrading()){
    qWarning("%s",tr("Can't clear while upgrading firmware").toUtf8().data());
    return;
  }
  stop();
  beginResetModel();
  root->clear();
  nodes.clear();
  endResetModel();
  requestActive=false;
  nodes_found_cnt=0;
  emit changed();
}
//=============================================================================
void NodesModel::sync(void)
{
  if(mandala->current!=mvar){
    stop();
    return;
  }
  //qDebug()<<"sync";
  //scan for nodes
  requestManager.makeRequest(apc_search,QByteArray(),QByteArray(),0);
  //re-request node_info of known nodes
  foreach(QByteArray sn,nodes.uniqueKeys())
    requestManager.makeRequest(apc_info,sn,QByteArray(),500);
  //request missing fields
  root->sync();
  emit syncProgress(0);
}
//=============================================================================
void NodesModel::stats(void)
{
  if(mandala->current!=mvar){
    stop();
    return;
  }
  root->stats();
}
//=============================================================================
void NodesModel::upload(void)
{
  root->upload();
  saveToTelemetry();
}
//=============================================================================
void NodesModel::stop(void)
{
  if(isUpgrading()){
    upgradeNodes.clear();
    root->stop();
  }
  requestManager.stop();
  emit syncProgress(0);
}
//=============================================================================
void NodesModel::requestManagerFinished()
{
  if(nodes.size()!=nodes_found_cnt){
    nodes_found_cnt=nodes.size();
    if(root->isValid())saveToTelemetry();
    sync();
  }
  emit done();
}
//=============================================================================
void NodesModel::upgradeFirmware(NodesList nodesList, UpgradeType utype)
{
  foreach(NodesItemNode *node,nodesList){
    if(!upgradeNodes[utype].contains(node))
      upgradeNodes[utype].append(node);
  }
  firmwareUpgraded();
}
void NodesModel::firmwareUpgraded()
{
  if(isUpgrading())return;
  else stop();
  if(!upgradeNodes[UpgradeLoader].isEmpty()){
    upgradeNodes[UpgradeLoader].takeFirst()->firmwareLoader.upgradeLoader();
    return;
  }
  if(!upgradeNodes[UpgradeFirmware].isEmpty()){
    upgradeNodes[UpgradeFirmware].takeFirst()->firmwareLoader.upgradeFirmware();
    return;
  }
  if(!upgradeNodes[UpgradeMHX].isEmpty()){
    upgradeNodes[UpgradeMHX].takeFirst()->firmwareLoader.upgradeMHX();
    return;
  }
  if(!isUpgradable())qDebug("%s",tr("Everything is up-to-date").toUtf8().data());
  else qDebug("%s",tr("Firmware upgrade finished").toUtf8().data());
  emit syncProgress(0);
  emit done();
  QTimer::singleShot(2000,&requestManager,SLOT(enableTemporary()));
  QTimer::singleShot(3000,this,SLOT(sync()));
}
//=============================================================================
void NodesModel::service(const QByteArray &packet_data)
{
  //qDebug()<<packet_data.toHex();
  _bus_packet &packet=*(_bus_packet*)packet_data.data();
  QByteArray sn((const char*)packet.srv.sn,sizeof(_node_sn));
  QByteArray data((const char*)packet.srv.data,packet_data.size()-bus_packet_size_hdr_srv);
  //continue processing
  //service(sn,packet.srv.cmd,data);
  switch(packet.srv.cmd){
    case apc_search:
      if(mvar->node_info.contains(sn) && (!nodes.contains(sn))){
        requestManager.makeRequest(apc_info,sn,QByteArray(),0);
      }
      return;
    case apc_nstat: break;
    case apc_info: {
      if(nodes.contains(sn))break;
      //new node found
      QByteArray ba=data;
      if(ba.size()<(int)sizeof(_node_name))return;
      if(ba.size()>(int)sizeof(_node_info))ba.resize(sizeof(_node_info));
      _node_info ninfo;
      memset(&ninfo,0,sizeof(_node_info));
      memcpy(&ninfo,ba.data(),sizeof(_node_info));
      create_node(sn,&ninfo);
    }break;
  }
  //pass to nodes items
  if(!nodes.contains(sn))return;
  nodes.value(sn)->response_received(packet.srv.cmd,data);
  if(requestManager.busy())emit syncProgress(root->sizePending());
}
//=============================================================================
void NodesModel::create_node(QByteArray sn, _node_info *ninfo)
{
  beginResetModel();
  NodesItemNode *node=new NodesItemNode(root,this,sn,ninfo);
  nodes.insert(sn,node);
  endResetModel();
  //check node grouping
  //find same names with same parent
  QList<NodesItem*>nlist;
  NodesItemNgrp *group=NULL;
  QString gname=node->name;
  QStringList names;
  names.append(node->name);
  if(node->name=="bldc"){
    gname="servo";
    names.append(gname);
  }
  foreach(NodesItem *i,node->parentItem->childItems){
    if(i->item_type==NodesItem::it_node){
      if(names.contains(i->name))
        nlist.append(i);
    }else if(i->item_type==NodesItem::it_ngroup){
      //qDebug()<<"scan: "<<i->name<<i->childCount()<<i->childItems.first()->name;
      if(i->childCount() && i->name==gname)
        group=static_cast<NodesItemNgrp*>(i);
    }
  }

  if(group==NULL && nlist.size()<2)return;
  //qDebug()<<"-append-";
  if(!group){
    group=new NodesItemNgrp(node->parentItem,gname);
    //qDebug()<<"grp: "<<gname;
  }
  beginResetModel();
  foreach(NodesItem *i,nlist){
    i->parentItem->removeChild(i);
    group->appendChild(i);
    //qDebug()<<gname<<"<<"<<i->name;
    //if(node->name.contains("shiva")) qDebug()<<node->name<<nlist.size()<<(group?group->name:"");
  }
  //update group descr
  QStringList gNames,gHW;
  foreach(NodesItem *i,group->childItems){
    if(i->item_type!=NodesItem::it_node)continue;
    NodesItemNode *n=static_cast<NodesItemNode*>(i);
    if(!gNames.contains(n->name))gNames.append(n->name);
    QString s=n->node_info.hardware[0]?QString("%1").arg((const char*)n->node_info.hardware):QString();
    if(!gHW.contains(s))gHW.append(s);
  }
  QStringList sdescr;
  if(!gNames.isEmpty())sdescr.append(gNames.join(','));
  if(!gHW.isEmpty())sdescr.append("("+gHW.join(',')+")");
  if(!sdescr.isEmpty())group->descr=sdescr.join(' ');

  endResetModel();
  emit syncProgress(root->size());
}
//=============================================================================
void NodesModel::cacheClear(void)
{
  QDir backup_dir(AppDirs::nodes());
  foreach(QString sname,backup_dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
    foreach(QString dsn,QDir(backup_dir.filePath(sname)).entryList(QDir::Dirs|QDir::NoDotAndDotDot))
      QFile::remove(backup_dir.filePath(QString("%1/%2/%3").arg(sname).arg(dsn).arg("cache")));
}
//=============================================================================
QString NodesModel::title() const
{
  //qDebug()<<mvar->objectName();
  return mvar->apcfg.value("comment").toString();
}
//=============================================================================
void NodesModel::saveToTelemetry(void)
{
  if(!nodes.size())return;
  QByteArray hash;
  foreach(NodesItemNode *i,nodes)
    hash.append(i->md5());
  if(telemetry_hash==hash)return;
  telemetry_hash=hash;

  QDomDocument doc("");
  saveToXml(doc);
  QBuffer b;
  b.open(QBuffer::WriteOnly);
  QTextStream stream(&b);
  stream<<"\n";
  doc.save(stream,2);
  QByteArray ba(b.data());
  if(ba.endsWith('\n'))ba.resize(ba.size()-1);
  mvar->rec->saveXmlPart("nodes",ba);
}
//=============================================================================
void NodesModel::saveToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("nodes"));
  dom.toElement().setAttribute("href","http://www.uavos.com/");
  dom.toElement().setAttribute("title",title());
  dom.toElement().setAttribute("version",FactSystem::version());
  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(QString(md5().toHex())));
  dom.appendChild(doc.createElement("timestamp")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));
  saveIdentToXml(dom);
  root->saveToXml(dom);
}
//=============================================================================
void NodesModel::saveToFile(QString fname) const
{
  QFile file(fname);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QDomDocument doc;
  doc.appendChild(doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\""));
  saveToXml(doc);
  QTextStream stream(&file);
  doc.save(stream,2);
  file.close();
}
//=============================================================================
void NodesModel::saveIdentToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  QDomNode e=dom.appendChild(doc.createElement("ident"));
  e.appendChild(doc.createElement("callsign")).appendChild(doc.createTextNode(mvar->ident.callsign));
  e.appendChild(doc.createElement("class")).appendChild(doc.createTextNode(QMandala::vclassToString(mvar->ident.vclass)));
  e.appendChild(doc.createElement("squawk")).appendChild(doc.createTextNode(QString::number(mvar->ident.squawk)));
  e.appendChild(doc.createElement("uid")).appendChild(doc.createTextNode(QString(QByteArray((const char*)mvar->ident.uid,sizeof(mvar->ident.uid)).toHex().toUpper())));
}
//=============================================================================
void NodesModel::saveVehicleBackup()
{
  if(!root->isValid())return;
  if(!(backup_dir_ok&&backup_dir.exists()))return;
  if(!backup_dir.isReadable())return;
  if(root->isReconf())return;

  QString fname=QDateTime::currentDateTime().toString("yyyy'_'MM'_'dd'_'hh'_'mm'_'ss'_'zzz")+".nodes";
  QFile file(backup_dir.filePath(fname));
  QDomDocument doc;
  doc.appendChild(doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\""));
  saveToXml(doc);
  if(!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QTextStream stream(&file);
  doc.save(stream,2);
  file.close();

  //check same file
  const QString md5s=md5().toHex();
  if(backup_sidx->contains(md5s)){
    QFile::remove(backup_dir.filePath(backup_sidx->value(md5s).toString()));
    //qDebug()<<"same file";
  }
  backup_sidx->setValue(md5s,fname);
  backup_dir.refresh();

  //delete old backups
  while(backup_dir.count()>20){
    fname=backup_dir.entryList().first();
    if(!QFile::remove(backup_dir.filePath(fname)))break;
    foreach(QString key,backup_sidx->allKeys()){
      if(backup_sidx->value(key).toString()!=fname)continue;
      backup_sidx->remove(key);
      break;
    }
    backup_dir.refresh();
  }
}
void NodesModel::restoreVehicleBackup(QString fname)
{
  if(!(backup_dir_ok&&backup_dir.exists(fname)))return;
  loadFromFile(backup_dir.filePath(fname));
}
//=============================================================================
//=============================================================================
QByteArray NodesModel::md5() const
{
  QByteArray ba;
  ba.append(QByteArray((const char*)&mvar->ident,sizeof(mvar->ident)));
  foreach(NodesItemNode *i,nodes.values())
    ba.append(i->md5());
  return QCryptographicHash::hash(ba,QCryptographicHash::Md5);
}
//=============================================================================
void NodesModel::loadFromTelemetry(void)
{
  QDomDocument doc;
  if(!mandala->local->rec->file.xmlParts.value("nodes").values().isEmpty())
    doc.setContent(mandala->local->rec->file.xmlParts.value("nodes").values().first());
  if(doc.documentElement().nodeName()!="nodes")return;
  clear();
  loadFromXml(doc.documentElement());
}
//=============================================================================
void NodesModel::replay_progress(uint time_ms)
{
  if(isEmpty())loadFromTelemetry();
  //read node_conf updates from telemetry player
  foreach(const QString &s,mandala->local->rec->file.xmlParts.value("node_conf").values(time_ms)){
    QDomDocument doc;
    doc.setContent(s);
    if(doc.documentElement().nodeName()!="node_conf")return;
    QDomNode dom=doc.documentElement();
    QDomElement e=dom.toElement();
    QString fname=e.attribute("f");
    QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
    if(nodes.contains(sn)){
      NodesItemNode *node=nodes.value(sn);
      //find field by conf_name
      foreach(NodesItemField *f,node->fields){
        if((!f->isValid())||f->conf_name!=fname)continue;
        f->loadFromXml(e);
        break;
      }
    }
    qDebug("[RE]%s: %s",tr("Field modified").toUtf8().data(),fname.toUtf8().data());
  }
}
//=============================================================================
void NodesModel::loadFromFile(const QString &fname)
{
  QFile file(fname);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot read file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QDomDocument doc;
  if(!(doc.setContent(&file) && doc.documentElement().nodeName()=="nodes")){
    qWarning("%s",QString(tr("The file format is not correct")).toUtf8().data());
    file.close();
    return;
  }
  file.close();
  int icnt=loadFromXml(doc.documentElement());
  if(nodes.size() && icnt!=nodes.size()){
    qDebug("%s",tr("Loaded %1 nodes of %2. Importing...").arg(icnt).arg(nodes.size()).toUtf8().data());
    uint icnt=importFromXml(doc.documentElement());
    if(!icnt){
      qDebug("%s",tr("Nodes didn't match, nothing to import").toUtf8().data());
      return;
    }
    qDebug("%s",tr("Imported %1 nodes").arg(icnt).toUtf8().data());
  }
}
//=============================================================================
int NodesModel::loadFromXml(QDomNode dom)
{
  int icnt=0;
  if(!nodes.size()){
    int pcnt=dom.childNodes().size();
    emit syncProgress(pcnt);
    //load and construct nodes from file
    dom.childNodes().size();
    beginResetModel();
    QDomElement e=dom.firstChildElement("node");
    while(!e.isNull()){
      QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
      QString name=e.attribute("name");
      QDomElement ei=e.firstChildElement("info");
      if(!ei.isNull()){
        _node_info ninfo;
        memset(&ninfo,0,sizeof(_node_info));
        memcpy(ninfo.name,name.toUtf8().data(),name.size()<(int)sizeof(ninfo.name)?name.size():sizeof(ninfo.name));
        QString s=ei.firstChildElement("version").text();
        memcpy(ninfo.version,s.toUtf8().data(),s.size()<(int)sizeof(ninfo.version)?s.size():sizeof(ninfo.version));
        s=ei.firstChildElement("hardware").text();
        memcpy(ninfo.hardware,s.toUtf8().data(),s.size()<(int)sizeof(ninfo.hardware)?s.size():sizeof(ninfo.hardware));
        create_node(sn,&ninfo);
        nodes.value(sn)->loadFromXml(e);
        icnt++;
      }
      e=e.nextSiblingElement(e.tagName());
      emit syncProgress(pcnt--);
      QCoreApplication::processEvents();
    }
    emit syncProgress(0);
    endResetModel();
    emit done();
    return icnt;
  }
  //load node conf values
  QDomElement e=dom.firstChildElement("node");
  while(!e.isNull()){
    QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
    if(nodes.contains(sn)){
      nodes.value(sn)->loadFromXml(e);
      icnt++;
    }
    e=e.nextSiblingElement(e.tagName());
  }
  return icnt;
}
//=============================================================================
bool NodesModel::isModified(void) const
{
  return root->isModified();
}
bool NodesModel::isValid(void) const
{
  return root->isValid();
}
bool NodesModel::isEmpty(void) const
{
  return !(root->childCount()&&nodes.size());
}
bool NodesModel::isUpgrading(void) const
{
  return root->isUpgrading();
}
bool NodesModel::isUpgradable(void) const
{
  return root->isUpgradable();
}
//=============================================================================
QVariant NodesModel::data(const QModelIndex &index, int role) const
{
  if(!index.isValid()) return QVariant();
  NodesItem *i=item(index);
  if(!i)return QVariant();
  if(role==NodesItemRole) return QVariant::fromValue<NodesItem*>(i);
  return i->data(index.column(),role);
}
//=============================================================================
bool NodesModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if ((!index.isValid()) || (role!=Qt::EditRole) || index.column()!=NodesItem::tc_value)
    return false;
  NodesItem *i = item(index);
  if(!i)return false;
  if (data(index,role)==value)return true;
  bool rv=i->setData(index.column(),value);
  //if(rv)emit dataChanged(index,index);//layoutChanged();
  return rv;
}
//=============================================================================
QModelIndex NodesModel::index(int row, int column, const QModelIndex &parent) const
{
  if(!hasIndex(row, column, parent)) return QModelIndex();
  NodesItem *parentItem;
  if(!parent.isValid()) parentItem = root;
  else parentItem = item(parent);
  if(!parentItem) return QModelIndex();
  NodesItem *childItem = parentItem->child(row);
  if(!childItem) return QModelIndex();
  QModelIndex i=createIndex(row, column, childItem);
  childItem->model_index=i;
  return i;
}
//=============================================================================
QModelIndex NodesModel::parent(const QModelIndex &index) const
{
  if(!index.isValid()) return QModelIndex();
  NodesItem *i=item(index);
  if(!i)return QModelIndex();
  NodesItem *parentItem = i->parent();
  if (!parentItem || parentItem == root) return QModelIndex();
  QModelIndex parentIndex=createIndex(parentItem->row(),0,parentItem);
  parentItem->model_index=parentIndex;
  return parentIndex;
}
//=============================================================================
int NodesModel::rowCount(const QModelIndex &parent) const
{
  NodesItem *parentItem;
  if (parent.column() > 0) return 0;
  if (!parent.isValid()) parentItem = root;
  else parentItem = item(parent);
  if(!parentItem->childCount())return 0;
  if(parentItem->item_type==NodesItem::it_group && static_cast<NodesItemGroup*>(parentItem)->isValueArray())return 0;
  return parentItem->childCount();
}
//=============================================================================
int NodesModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid()) return item(parent)->columnCount();
  else return root->columnCount();
}
//=============================================================================
QVariant NodesModel::headerData(int section, Qt::Orientation orientation,int role) const
{
  if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return root->data(section);
  return QVariant();
}
//=============================================================================
Qt::ItemFlags NodesModel::flags(const QModelIndex & index) const
{
  if (!index.isValid()) return 0;
  return item(index)->flags(index.column());
}
//=============================================================================
QModelIndexList NodesModel::getPersistentIndexList()
{
  return persistentIndexList();
}
NodesItem *NodesModel::item(const QModelIndex &index) const
{
  return static_cast<NodesItem*>(index.internalPointer());
}
QModelIndexList NodesModel::getGroupsIndexList(void)
{
  QModelIndexList list;
  foreach(NodesItem *i,root->childItems){
    if(i->item_type==NodesItem::it_ngroup){
      list.append(i->model_index);
    }
  }
  return list;
}
//=============================================================================
void NodesModel::itemDataChanged(NodesItem *item)
{
  QVector<int> roles;
  roles<<Qt::DisplayRole<<Qt::ForegroundRole;
  while(item){
    if(item->model_index.isValid())
      dataChanged(item->model_index,item->model_index,roles);
    item=item->parentItem;
  }
  emit changed();
}
//=============================================================================
//=============================================================================
uint NodesModel::importFromXml(QDomNode dom)
{
  int icnt=0;
  QHash<NodesItemNode*,QByteArray> snmap;
  QHash<NodesItemNode*,int>nmap;
  int priority=0;
  QDomNodeList nlist=dom.toElement().elementsByTagName("node");
  for(int i=0;i<nlist.length();i++){
    QDomNode ne=nlist.at(i);
    //qDebug()<<node.toElement().attribute("sn");
    QString ssn=ne.toElement().attribute("sn");
    QString node_name=ne.toElement().attribute("name");
    QString comment;
    //find comment from file
    QDomElement nfield=ne.firstChildElement("fields").firstChildElement("field");
    for(; !nfield.isNull(); nfield = nfield.nextSiblingElement("field")) {
      if(nfield.attribute("name")=="comment"){
        comment=nfield.firstChildElement("value").text();
        break;
      }
    }
    //qDebug()<<ssn<<node_name<<comment;
    NodesItemNode *node=NULL;
    const QByteArray &sn=QByteArray::fromHex(ssn.toUtf8());
    //match by serial numner
    priority=0;
    if(nodes.contains(sn)){
      node=nodes.value(sn);
      nmap.insert(node,priority);
      //qDebug()<<"Node by sn: "<<node->name<<node_name<<comment;
    }
    //find matching node by name and comment
    priority++;
    if(!node){
      if(comment.size()){
        foreach(NodesItemNode *i,nodes.values())
          if(i->name==node_name && i->valueByName("comment").toString()==comment){
            if(nmap.contains(i) && nmap.value(i)<=priority)continue;
            node=i;
            nmap.insert(node,priority);
            //qDebug()<<"Node by name+comment: "<<node->name<<node_name<<comment;
            break;
          }
      }
    }//else qDebug()<<"Node by sn: "<<node->name<<node_name<<comment;
    //find matching node by name without comment
    priority++;
    if(!node){
      if(node_name!="servo"){
        foreach(NodesItemNode *i,nodes.values())
          if(i->name==node_name && i->valueByName("comment").toString().isEmpty()){
            if(nmap.contains(i) && nmap.value(i)<=priority)continue;
            node=i;
            nmap.insert(node,priority);
            //qDebug()<<"Node by name-no-comment: "<<node->name<<node_name<<comment;
            break;
          }
      }
    }
    //find matching node by name with any comment
    priority++;
    if(!node){
      if(node_name!="servo"){
        foreach(NodesItemNode *i,nodes.values())
          if(i->name==node_name){
            if(nmap.contains(i) && nmap.value(i)<=priority)continue;
            node=i;
            nmap.insert(node,priority);
            //qDebug()<<"Node by name: "<<node->name<<node_name<<comment;
            break;
          }
      }
    }
    //find matching node by name part
    priority++;
    if(!node){
      if(node_name.contains('.')){
        node_name.remove(0,node_name.indexOf('.'));
        foreach(NodesItemNode *i,nodes.values())
          if(i->name.endsWith(node_name)){
            if(nmap.contains(i) && nmap.value(i)<=priority)continue;
            node=i;
            nmap.insert(node,priority);
            //qDebug()<<"Node by name part: "<<node->name<<node_name<<comment;
            break;
          }
      }
    }
    if(!node)continue; //no matching nodes
    snmap.insert(node,sn);
    //node->loadFromXml(ne);
    icnt++;
  }
  for(int i=0;i<nlist.length();i++){
    QDomNode ne=nlist.at(i);
    const QByteArray &sn=QByteArray::fromHex(ne.toElement().attribute("sn").toUtf8());
    NodesItemNode *node=snmap.key(sn,NULL);
    if(node)node->loadFromXml(ne);
  }
  return icnt;
}
//=============================================================================
//=============================================================================


