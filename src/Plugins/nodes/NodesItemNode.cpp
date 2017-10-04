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
#include "NodesModel.h"
#include "NodesItemNode.h"
#include "NodesItemField.h"
#include "QMandala.h"
#include "BlackboxDownload.h"
//=============================================================================
NodesItemNode::NodesItemNode(NodesItem *parent, NodesModel *model, QByteArray sn, _node_info *ninfo)
 : NodesItem(parent),sn(sn),
   inf_valid(false),cmd_valid(false),model(model),
   firmwareLoader(this),
   backup_dir_ok(false),valid_state_s(false)
{
  item_type=it_node;

  blackboxDownload=NULL;

  memcpy(&node_info,ninfo,sizeof(_node_info));
  memset(&node_status,0,sizeof(_node_status));
  statsInit=false;

  name=QString((char*)node_info.name);
  //init backup directory
  backup_dir.setPath(QMandala::Global::nodes().path());
  QString spath=QString("%1/%2").arg(name).arg(QString(sn.toHex().toUpper()));
  if(!(backup_dir.mkpath(spath)&&backup_dir.cd(spath))){
    qWarning("%s",tr("Can't create backup path").toUtf8().data());
  }else{
    backup_dir.setNameFilters(QStringList()<<"*.nodes");
    backup_dir.setSorting(QDir::Name);
    backup_dir_ok=true;
    backup_sidx=new QSettings(backup_dir.filePath("index"),QSettings::IniFormat);
  }
  memset(&conf_inf,0,sizeof(conf_inf));
  sync_apcfg_timer.setSingleShot(true);
  connect(&sync_apcfg_timer,SIGNAL(timeout()),this,SLOT(sync_apcfg()));

  chk_inf_timer.setSingleShot(true);
  chk_inf_timer.setInterval(1000);
  connect(&chk_inf_timer,SIGNAL(timeout()),this,SLOT(chk_inf()));

  save_bkp_timer.setSingleShot(true);
  save_bkp_timer.setInterval(1000);
  connect(&save_bkp_timer,SIGNAL(timeout()),this,SLOT(saveBackupFile()),Qt::QueuedConnection);

  connect(this,SIGNAL(request(uint,QByteArray,QByteArray,uint)),&model->requestManager,SLOT(makeRequest(uint,QByteArray,QByteArray,uint)));

  statsShowTimer.setSingleShot(true);
  statsShowTimer.setInterval(30000);
  connect(&statsShowTimer,SIGNAL(timeout()),this,SLOT(updateModelData()));
}
NodesItemNode::~NodesItemNode()
{
  delete backup_sidx;
}
//=============================================================================
QVariant NodesItemNode::data(int column,int role) const
{
  switch(role){
  case Qt::ForegroundRole:
    if(isUpgrading())return QColor(Qt::white);
    if(!isValid())return column==tc_field?QVariant():QColor(Qt::darkGray);
    if(column==tc_field){
      if(isModified())return QColor(Qt::red).lighter();
      return QVariant();
    }
    if(column==tc_value)return QColor(Qt::yellow);//QVariant();
    if(!statsShowTimer.isActive()){
      return isUpgradable()?QColor(Qt::red).lighter():Qt::darkGray;
    }
    //nstats
    return statsWarn?QColor(Qt::yellow):QColor(Qt::green);
  case Qt::BackgroundRole:
    if(isUpgrading())return QColor(0x20,0x00,0x00);
    if(isUpgradePending())return QColor(0x40,0x00,0x00);
    if(!inf_valid)return QVariant();//QColor(Qt::red).lighter();
    if(!isValid())return QColor(50,50,0);
    if(node_info.flags.conf_reset)return QColor(Qt::darkGray).darker(200);
    return QColor(Qt::darkCyan).darker(200);//QColor(0x20,0x40,0x40);//isModified()?QColor(0x40,0x20,0x20):
  case Qt::FontRole:
    return QFont("Monospace",-1,column==(tc_field&&inf_valid)?QFont::Bold:QFont::Normal);
  case Qt::ToolTipRole:
    if(column==tc_field){
      QStringList st;
      st<<name;
      st<<QString("[%1 %2]").arg(getConfSize()).arg(tr("bytes"));
      st<<QString("[%1 %2]").arg(fields.size()).arg(tr("fields"));
      st<<QString("%1: %2").arg(tr("conf reset")).arg(node_info.flags.conf_reset?tr("yes"):tr("no"));
      st<<QString("%1: %2").arg(tr("loader support")).arg(node_info.flags.loader_support?tr("yes"):tr("no"));
      st<<QString("%1: %2").arg(tr("in loader")).arg(node_info.flags.in_loader?tr("yes"):tr("no"));
      st<<QString("%1: %2").arg(tr("addressing")).arg(node_info.flags.addressing?tr("yes"):tr("no"));
      st<<QString("%1: %2").arg(tr("reboot")).arg(node_info.flags.reboot?tr("yes"):tr("no"));
      st<<QString("%1: %2").arg(tr("busy")).arg(node_info.flags.busy?tr("yes"):tr("no"));
      return st.join('\n');
    }
    if(column!=tc_descr)return QVariant();
    if(statsShowTimer.isActive()){
      QStringList st;
      st<<QString().sprintf("%s:\t%.2X",tr("CAN address").toUtf8().data(),node_status.can_adr);
      st<<QString().sprintf("%s:\t%.2X",tr("CAN rx packets").toUtf8().data(),node_status.can_rxc);
      st<<QString().sprintf("%s:\t%.2X",tr("System errors").toUtf8().data(),node_status.err_cnt);
      st<<QString().sprintf("%s:\t%.2X",tr("CAN errors").toUtf8().data(),node_status.can_err);
      st<<QString().sprintf("%s:\t%u%%",tr("MCU load").toUtf8().data(),(100*(uint)node_status.load)/255);
      if(node_status.power.VBAT)
        st<<QString().sprintf("%s:\t%.1fV %umA",tr("Power").toUtf8().data(),node_status.power.VBAT/1000.0,node_status.power.IBAT);
      return st.join('\n');
    }
    return data(column,Qt::DisplayRole);
  }
  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(column){
    case tc_descr: {
      if(isUpgrading())return firmwareLoader.status();
      if(!statsShowTimer.isActive()){
        QString ver=node_info.version[0]?QString("%1").arg((const char*)node_info.version):QString("<"+tr("LOADER")+">");
        QString hw=node_info.hardware[0]?QString("%1").arg((const char*)node_info.hardware):QString();
        return QString("%1 %2 %3").arg(hw,-5).arg(ver).arg(model->mvar->node_sn(sn));
      }
      //show stats
      QString s;
      s+=QString().sprintf("%.2X:%.2X E:%.2X C:%.2X %*u%%",node_status.can_adr,node_status.can_rxc,node_status.err_cnt,node_status.can_err,2,(100*(uint)node_status.load)/255);
      if(node_status.power.VBAT)
        s+=QString().sprintf(" %.1fV %umA",node_status.power.VBAT/1000.0,node_status.power.IBAT);
      return s;
    }
  }
  return NodesItem::data(column,role);
}
//=============================================================================
void NodesItemNode::updateModelData()
{
  model->itemDataChanged(this);
}
//=============================================================================
QVariant NodesItemNode::getValue(void) const
{
  return valueByName("comment");
}
//=============================================================================
bool NodesItemNode::isValid(void) const
{
  return inf_valid && cmd_valid&&NodesItem::isValid();
}
//=============================================================================
bool NodesItemNode::isReconf(void) const
{
  return inf_valid && node_info.flags.conf_reset;
}
//=============================================================================
bool NodesItemNode::isModified(void) const
{
  if(!isValid())return false;
  return NodesItem::isModified();
}
//=============================================================================
bool NodesItemNode::isUpgrading(void) const
{
  return firmwareLoader.busy;
}
//=============================================================================
bool NodesItemNode::isUpgradable(void) const
{
  return QMandala::version!=QString((const char*)node_info.version);
}
//=============================================================================
bool NodesItemNode::isUpgradePending(void) const
{
  foreach(const NodesList list,model->upgradeNodes.values()){
    foreach(const NodesItemNode *i,list){
      if(i==this)return true;
    }
  }
  return false;
}
//=============================================================================
Qt::ItemFlags NodesItemNode::flags(int column) const
{
  Qt::ItemFlags f=NodesItem::flags(column);
  if(column==tc_descr)f|=Qt::ItemIsEditable;
  if(!isUpgrading())f|=Qt::ItemIsEnabled;
  return f;
}
//=============================================================================
//=============================================================================
QVariant NodesItemNode::valueByName(QString vname) const
{
  foreach(NodesItem *i,fields)
    if(i->name==vname)return i->getValue();
  return QVariant();
}
//=============================================================================
int NodesItemNode::getConfSize(void) const
{
  if(!inf_valid)return 0;
  return conf_inf.size;
}
//=============================================================================
QByteArray NodesItemNode::md5() const
{
  QByteArray ba;
  ba.append(QByteArray((const char*)&node_info,sizeof(_node_info)));
  ba.append(QByteArray((const char*)&conf_inf,sizeof(_conf_inf)));
  foreach(NodesItemField *f,fields) ba.append(f->data());
  return QCryptographicHash::hash(ba,QCryptographicHash::Md5);
}
//=============================================================================
uint NodesItemNode::progress() const
{
  if(isUpgrading())return firmwareLoader.progress();
  int cnt=0;
  if(fields.size()){
    int dsc_cnt=0;
    foreach(const NodesItem *i,fields)
      if(i->isValid()){
        cnt++;
        dsc_cnt++;
      }else if(static_cast<const NodesItemField*>(i)->dsc_valid)
        dsc_cnt++;
    if(cnt==fields.size())cnt=0;
    else if(dsc_cnt==fields.size())cnt=cnt*100/fields.size();
    else cnt=(dsc_cnt+cnt)*100/(fields.size()*2);
  }
  return cnt;
}
//=============================================================================
void NodesItemNode::saveToXml(QDomNode dom) const
{
  //QCoreApplication::processEvents();
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("node"));
  dom.toElement().setAttribute("sn",QString(sn.toHex().toUpper()));
  dom.toElement().setAttribute("name",name);

  QDomNode e=dom.appendChild(doc.createElement("info"));
  e.appendChild(doc.createElement("version")).appendChild(doc.createTextNode(QString((const char*)node_info.version)));
  e.appendChild(doc.createElement("hardware")).appendChild(doc.createTextNode(QString((const char*)node_info.hardware)));

  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(QString(md5().toHex())));
  dom.appendChild(doc.createElement("timestamp")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));

  if(commands.cmd.size()){
    for(int i=0;i<commands.cmd.size();i++){
      QDomNode e=dom.appendChild(doc.createElement("command"));
      e.toElement().setAttribute("cmd",QString::number(commands.cmd.at(i)));
      e.appendChild(doc.createElement("name")).appendChild(doc.createTextNode(commands.name.at(i)));
      e.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(commands.descr.at(i)));
    }
  }

  e=dom.appendChild(doc.createElement("fields"));
  e.toElement().setAttribute("cnt",QString::number(conf_inf.cnt));
  e.toElement().setAttribute("size",QString::number(conf_inf.size));
  e.toElement().setAttribute("mn",QString::number(conf_inf.mn));
  foreach(NodesItemField *f,fields){
    f->saveToXml(e);
  }
}
//=============================================================================
void NodesItemNode::loadFromXml(QDomNode dom)
{
  //dom is "node" tag
  bool bLoadStruct=!isValid();//(inf_valid&&fields.size());
  if(bLoadStruct){
    commands.cmd.clear();
    commands.name.clear();
    commands.descr.clear();
    cmd_valid=false;
  }
  int lcnt=0;
  if(bLoadStruct){
    //fields inf struct
    QDomElement e=dom.firstChildElement("fields");
    if(e.isNull())return; //no struct exists in dom
    //check mn in dom and this node
    uint cnt=e.attribute("cnt").toUInt();
    uint size=e.attribute("size").toUInt();
    uint mn=e.attribute("mn").toUInt();
    if(!fields.size()){
      conf_inf.cnt=cnt;
      conf_inf.size=size;
      conf_inf.mn=mn;
      create_fields();
    }else if(cnt!=conf_inf.cnt || size!=conf_inf.size || mn!=conf_inf.mn){
      //conf_inf in file and node not the same
      return;
    }
    //commands
    e=dom.firstChildElement("command");
    while(!e.isNull()){
      uint cmd=e.attribute("cmd").toUInt();
      QString sname=e.firstChildElement("name").text();
      QString sdescr=e.firstChildElement("descr").text();
      if(sname.size()&&sdescr.size()){
        commands.cmd.append(cmd);
        commands.name.append(sname);
        commands.descr.append(sdescr);
        cmd_valid=true;
        //qDebug()<<cmd<<sname<<sdescr;
      }
      e=e.nextSiblingElement(e.tagName());
    }
  }
  //read fields values
  QDomElement e=dom.firstChildElement("fields").firstChildElement("field");
  while(!e.isNull()){
    NodesItemField *f=NULL;
    QString fname=e.attribute("name");
    if(bLoadStruct){
      int fnum=e.attribute("idx").toUInt();
      if(fnum<fields.size())f=fields.at(fnum);
    }else{
      //find field by conf_name
      foreach(NodesItemField *i,fields){
        if((!i->isValid())||i->conf_name!=fname)continue;
        f=i;
        break;
      }
      //try find excl array size []
      if((!f) && fname.contains('[')){
        foreach(NodesItemField *i,fields){
          if(!i->isValid())continue;
          if(!i->conf_name.contains('['))continue;
          if(i->conf_name.left(i->conf_name.indexOf('['))!=fname.left(fname.indexOf('[')))continue;
          f=i;
          break;
        }
      }
      //find field by name substitution
      if(!f){
        QString vname=fname;
        if(vname.contains("_"))
          vname.remove(0,vname.indexOf('_')+1);
        if(vname!="comment"){
          foreach(NodesItemField *i,fields){
            if((!i->isValid())||i->conf_name!=vname)continue;
            f=i;
            break;
          }
        }
      }
      //anyway, try old formats
      int old_cnt=loadFromOldFormat(e);
      if(old_cnt)lcnt+=old_cnt;
      else if(!f) qWarning("%s: %s/%s",tr("Field missing").toUtf8().data(),name.toUtf8().data(),fname.toUtf8().data());
    }
    if(f){
      lcnt++;
      if((!bLoadStruct) && fname!=f->conf_name){
        qWarning("%s: %s/%s -> %s",tr("Field map").toUtf8().data(),name.toUtf8().data(),fname.toUtf8().data(),f->conf_name.toUtf8().data());
      }
      f->loadFromXml(e);
    }
    e=e.nextSiblingElement(e.tagName());
  }
  if(lcnt && lcnt==fields.size()){
    inf_valid=cmd_valid=true;
    //qDebug()<<lcnt;
  }
}
//=============================================================================
int NodesItemNode::loadFromOldFormat(QDomNode e)
{
  int lcnt=0;
  //try to map from old format
  QString fname=e.toElement().attribute("name");
  QString map_name;
  QString stype=e.firstChildElement("struct").firstChildElement("type").text();
  if(stype=="ctr"){
    foreach(NodesItemField *i,fields){
      if(!i->isValid())continue;
      if(i->conf_name.startsWith("ctr_bind[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("var").text());
        }
      }else if(i->conf_name.startsWith("ctr_mult[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("mult").text());
        }
      }else if(i->conf_name.startsWith("ctr_diff[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("diff").text());
        }
      }else if(i->conf_name.startsWith("ctr_speed[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("speed").text());
        }
      }else if(i->conf_name.startsWith("ctr_ch[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("pwm_ch").text());
        }
      }
    }
  }else if(stype=="pwm"){
    foreach(NodesItemField *i,fields){
      if(!i->isValid())continue;
      if(i->conf_name.startsWith("ctr_ch_min[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("min").text().toFloat()*(100.0/127.0));
        }
      }else if(i->conf_name.startsWith("ctr_ch_max[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("max").text().toFloat()*(100.0/127.0));
        }
      }else if(i->conf_name.startsWith("ctr_ch_zero[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("zero").text().toFloat()*(100.0/127.0));
        }
      }
    }
  }else if(stype=="gpio"){
    foreach(NodesItemField *i,fields){
      if(!i->isValid())continue;
      if(i->conf_name.startsWith("gpio_bind[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("var").text());
        }
      }else if(i->conf_name.startsWith("gpio_mode[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("opt").text().toUInt()&0x7F);
        }
      }else if(i->conf_name.startsWith("gpio_inv[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue((ev.firstChildElement("opt").text().toUInt()&0x80)?1:0);
        }
      }else if(i->conf_name.startsWith("gpio_mult[")){
        lcnt++;
        for(QDomElement ev=e.firstChildElement("value");!ev.isNull();ev=ev.nextSiblingElement(ev.tagName())){
          int idx=ev.attribute("idx").toUInt();
          if(idx>=i->childCount())continue;
          i->child(idx)->setValue(ev.firstChildElement("mult").text());
        }
      }
    }
  }else if(name=="mhx" && fname=="fn")map_name="function";
  else if(fname=="srv_enc")map_name="encoder";
  else if(fname=="srv_erev")map_name="enc_rev";
  else if(fname=="srv_encbias")map_name="enc_bias";
  else if(fname=="srv_scale")map_name="enc_scale";
  else if(fname=="srv_flt")map_name="enc_flt";
  else if(fname=="srv_360")map_name="enc_360";
  else if(fname=="srv_pwm")map_name="m_pwm";
  else if(fname=="srv_pwm_min")map_name="m_pwm_min";
  else if(fname=="ld_flareVS")map_name="ld_flareDS";
  else if(fname=="to_taxiSpeed")map_name="taxi_gSpeed";
  else if(fname=="name")map_name="comment";

  if(!map_name.isEmpty()){
    foreach(NodesItemField *i,fields){
      if((!i->isValid())||i->conf_name!=map_name)continue;
      if(i->conf_name=="comment")continue;
      qWarning("%s: %s/%s -> %s",tr("Field map").toUtf8().data(),name.toUtf8().data(),fname.toUtf8().data(),i->conf_name.toUtf8().data());
      i->loadFromXml(e);
      lcnt++;
      break;
    }
  }
  return lcnt;
}
//=============================================================================
bool NodesItemNode::differentToXmlFile(QString fname) const
{
  QFile file(fname);
  if(!file.open(QFile::ReadOnly | QFile::Text))return true;

  QDomDocument doc;
  if(!(doc.setContent(&file) && doc.documentElement().nodeName()=="nodes")){
    return true;
  }

  QDomElement e=doc.documentElement().firstChildElement("node");
  QString ssn=sn.toHex().toUpper();
  while(!e.isNull()){
    QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
    if(e.attribute("sn")==ssn){
      return QByteArray::fromHex(e.firstChildElement("hash").text().toUtf8())!=md5();
    }
    e=e.nextSiblingElement(e.tagName());
  }
  return true;
}
//=============================================================================
void NodesItemNode::saveToCache(void)
{
  if(!(backup_dir_ok&&backup_dir.exists()))return;
  QString fname=backup_dir.filePath("cache");
  QFile file(fname);
  if(!file.open(QFile::WriteOnly | QFile::Text)) {
    qWarning("%s",QString(tr("Cannot write file")+" %1:\n%2.").arg(fname).arg(file.errorString()).toUtf8().data());
    return;
  }
  QDomDocument doc;
  doc.appendChild(doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\""));
  QDomNode dom=doc.appendChild(doc.createElement("nodes"));
  dom.toElement().setAttribute("href","http://www.uavos.com/");
  dom.toElement().setAttribute("title",QFileInfo(fname).baseName());
  dom.toElement().setAttribute("version",QMandala::version);
  saveToXml(dom);
  QTextStream stream(&file);
  doc.save(stream,2);
  file.close();
}
//=============================================================================
void NodesItemNode::loadFromCache(void)
{
  if(!(backup_dir_ok&&backup_dir.exists()))return;
  QString fname="cache";
  if(!backup_dir.exists(fname))return;
  QFile file(backup_dir.filePath(fname));
  if(!file.open(QFile::ReadOnly | QFile::Text))return;

  QDomDocument doc;
  if(!(doc.setContent(&file) && doc.documentElement().nodeName()=="nodes")){
    return;
  }

  QDomElement e=doc.documentElement().firstChildElement("node");
  QString ssn=sn.toHex().toUpper();
  while(!e.isNull()){
    QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
    if(e.attribute("sn")==ssn){
      loadFromXml(e);
      invalidate();
    }
    e=e.nextSiblingElement(e.tagName());
  }
}
//=============================================================================
void NodesItemNode::clear(void)
{
  model->beginResetModel();
  fields.clear();
  NodesItem::clear();
  commands.cmd.clear();
  commands.name.clear();
  commands.descr.clear();
  inf_valid=false;
  cmd_valid=false;
  model->endResetModel();
  emit changed();
}
//=============================================================================
void NodesItemNode::stop(void)
{
  firmwareLoader.stop();
}
//=============================================================================
void NodesItemNode::field_changed()
{
  sync_apcfg_timer.start(1000);
}
//=============================================================================
void NodesItemNode::sync_apcfg()
{
  sync_apcfg_scr(this,model->mvar->engine.evaluate("conf"));
  //update apcfg fields in Mandala
  if(! (name.contains(".shiva") || name == "shiva") )return;
  foreach(NodesItemField *i,fields){
    model->mvar->apcfg[i->conf_name]=i->getValue();
    //if(mandala->isLocal() && i->conf_name=="name")
      //mandala->setUavName(i->getValue().toString());
  }
  model->mvar->apcfgUpdated();
  //qDebug("sync_apcfg (%s)",model->mvar->apcfg.value("name").toString().toUtf8().data());
}
void NodesItemNode::sync_apcfg_scr(NodesItem *item,QScriptValue mobj)
{
  if(item->childCount()){
    QScriptValue gobj=model->mvar->engine.newObject();
    QString s=item->name;
    if(item==this){
      if(s.contains(".shiva"))s="shiva";
      else{
        uint cnt=0;
        foreach(NodesItem *i,parent()->childItems)
          if(i->name==s)cnt++;
        if(cnt>1) s+="_"+sn.toHex().toUpper();
      }
    }
    mobj.setProperty(s,gobj);
    foreach(NodesItem *i,item->childItems)
      sync_apcfg_scr(i,gobj);
    return;
  }
  mobj.setProperty(item->name,model->mvar->engine.newQObject(item,QScriptEngine::QtOwnership,QScriptEngine::ExcludeSuperClassMethods|QScriptEngine::ExcludeSuperClassProperties));
}
//=============================================================================
void NodesItemNode::saveBackupFile(void)
{
  if(!isValid())return;
  if(!(backup_dir_ok&&backup_dir.exists()))return;
  if(!backup_dir.isReadable())return;
  //save cache
  saveToCache();

  if(isReconf())return;

  //check version
  //QString node_version((const char*)node_info.version);
  if(isUpgradable()){
    if(!QMandala::Global::devMode())
      qWarning("%s %s: %s",tr("Version").toUtf8().data(),node_info.name,node_info.version);
    /*if(!(QMandala::Global::devMode() || node_version.contains('-'))){
      qWarning("%s",tr("Backup not saved").toUtf8().data());
      return;
    }*/
  }
  //check same file
  const QString md5s=md5().toHex();
  if(backup_sidx->contains(md5s)){
    return;
  }
  QString fname=QDateTime::currentDateTime().toString("yyyy'_'MM'_'dd'_'hh'_'mm'_'ss'_'zzz")+".nodes";
  backup_sidx->setValue(md5s,fname);
  QFile::copy(backup_dir.filePath("cache"),backup_dir.filePath(fname));
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
//=============================================================================
void NodesItemNode::restoreBackupFile(QString fname)
{
  if(!(backup_dir_ok&&backup_dir.exists(fname)))return;
  QFile file(backup_dir.filePath(fname));
  if(!file.open(QFile::ReadOnly | QFile::Text))return;

  QDomDocument doc;
  if(!(doc.setContent(&file) && doc.documentElement().nodeName()=="nodes")){
    qWarning("%s",QString(tr("The file format is not correct")).toUtf8().data());
    return;
  }
  if(restoreBackupXml(doc.documentElement()))
    qDebug("%s '%s': %s",tr("Backup restored for").toUtf8().data(),name.toUtf8().data(),QFileInfo(fname).baseName().toUtf8().data());
}
//=============================================================================
bool NodesItemNode::restoreBackupXml(QDomNode dom)
{
  QDomNodeList nlist=dom.toElement().elementsByTagName("node");
  if(!nlist.size())return false;
  QString ssn=sn.toHex().toUpper();
  for(int i=0;i<nlist.size();i++){
    QDomNode e=nlist.at(i);
    if(e.toElement().attribute("sn")!=ssn)continue;
    loadFromXml(e);
    return true;
  }
  return false;
}
//=============================================================================
QString NodesItemNode::restoreRecentBackup(void)
{
  if(!(backup_dir_ok&&backup_dir.count()))return "";
  QStringList lst=backup_dir.entryList();
  for(int i=0;i<lst.size();i++){
    QString fname=lst.at(lst.size()-1-i);
    QFile file(backup_dir.filePath(fname));
    if(!file.open(QFile::ReadOnly | QFile::Text))continue;
    QDomDocument doc;
    if(!(doc.setContent(&file) && doc.documentElement().nodeName()=="nodes"))
      continue;
    if(!restoreBackupXml(doc.documentElement()))continue;
    //if(valueByName("comment").toString().isEmpty())continue;
    if(!isModified())continue;
    qDebug("%s '%s': %s",tr("Backup restored for").toUtf8().data(),name.toUtf8().data(),QFileInfo(fname).baseName().toUtf8().data());
    return fname;
  }
  restore();
  return "";
}
//=============================================================================
void NodesItemNode::sync(void)
{
  if(node_info.flags.in_loader)return;
  if(!inf_valid){
    clear();
    emit changed();
    emit request(apc_conf_inf,sn,QByteArray(),100);
    return;
  }
  if(!cmd_valid){
    emit request(apc_conf_cmds,sn,QByteArray(),500);
    //return;
  }
  NodesItem::sync();
  //request fields data if any
  foreach(NodesItemField *f,fields){
    if(f->isValid())continue;
    emit request(apc_conf_read,sn,QByteArray().append((unsigned char)f->fnum),500);
    break;
  }
}
//=============================================================================
void NodesItemNode::upload(void)
{
  if(isUpgrading()||isUpgradePending())return;
  if(!isValid()){
    sync();
    return;
  }
  if(!isModified())return;
  saveBackupFile();
  NodesItem::upload();
  emit request(apc_conf_write,sn,QByteArray().append((unsigned char)0xFF),1000);
}
//=============================================================================
void NodesItemNode::stats(void)
{
  if(!isValid()){
    sync();
    return;
  }
  emit request(apc_nstat,sn,QByteArray(),0);
}
//=============================================================================
void NodesItemNode::command(int cmd_idx)
{
  if(cmd_idx>=commands.cmd.size()){
    qWarning("%s",tr("Can't send command (unknown command)").toUtf8().data());
    sync();
    return;
  }
  QString cmd_name=commands.name.at(cmd_idx);
  if(cmd_name=="bb_read"){
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
  }

  //qDebug("cmd (%u)",cmd);
  emit request(commands.cmd.at(cmd_idx),sn,QByteArray(),500);
  if(commands.cmd.at(cmd_idx)==apc_reconf || commands.name.at(cmd_idx).startsWith("conf")) invalidate();
}
//=============================================================================
void NodesItemNode::create_fields(void)
{
  for(uint i=0;i<conf_inf.cnt;i++){
    NodesItemField *f=new NodesItemField(this,this,i);
    fields.append(f);
    connect(f,SIGNAL(request(uint,QByteArray,QByteArray,uint)),this,SIGNAL(request(uint,QByteArray,QByteArray,uint)));
  }
}
//=============================================================================
void NodesItemNode::response_received(unsigned char cmd,const QByteArray data)
{
  switch(cmd){
    case apc_nstat: {
      if(data.size()!=(sizeof(_node_name)+sizeof(_node_status)))return; //size error
      uint err_cnt=node_status.err_cnt;
      uint can_err=node_status.can_err;
      memcpy(&node_status,data.data()+sizeof(_node_name),sizeof(_node_status));
      statsWarn=statsInit && (err_cnt!=node_status.err_cnt || can_err!=node_status.can_err);
      statsInit=true;
      statsShowTimer.start();
      updateModelData();
    }return;
    case apc_info: {
      if(!data.size())return;  //request
      QByteArray ba=data;
      if(ba.size()<(int)sizeof(_node_name))return;
      if(ba.size()>(int)sizeof(_node_info))ba.resize(sizeof(_node_info));
      _node_info ninfo;
      memset(&ninfo,0,sizeof(_node_info));
      memcpy(&ninfo,ba.data(),(ba.size()>(int)sizeof(_node_info))?sizeof(_node_info):ba.size());
      statsShowTimer.stop();
      if(ninfo.flags.in_loader) return;
      if(node_info.flags.in_loader || memcmp(&node_info,&ninfo,sizeof(_node_info)-sizeof(_node_info::flags))!=0){
        clear();
      }else if(ninfo.flags.conf_reset!=node_info.flags.conf_reset){
        invalidate();
      }
      memcpy(&node_info,&ninfo,sizeof(_node_info));
      model->itemDataChanged(this);
      emit changed();
      if(inf_valid)
        chk_inf_timer.start();//confirm & recheck
      sync();
    }break;
    case apc_conf_inf:{
      if(!data.size())return;  //request
      if(data.size()!=sizeof(_conf_inf)){
        qWarning("Error conf_inf descriptor received from %s.",model->mvar->node_name(sn));
        break;
      }
      if(inf_valid && memcmp(&conf_inf,data.data(),sizeof(conf_inf))==0)break;
      memcpy(&conf_inf,data.data(),sizeof(conf_inf));
      //qDebug("apc_conf_inf %s (cnt: %u, size: %u, mn: %.8X)",model->mvar->node_name(sn),conf_inf.cnt,conf_inf.size,conf_inf.mn);
      clear();
      inf_valid=true;
      create_fields(); //create invalid fields to force requests
      loadFromCache(); //try to load from cache
      emit changed();
      sync();
    }break;
    case apc_conf_cmds: {
      //qDebug("apc_conf_cmds (cnt:%u)",data.size());
      if(!cmd_valid){
        commands.cmd.clear();
        commands.name.clear();
        commands.descr.clear();
        const char *str=(const char*)data.data();
        int cnt=data.size();
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
        }else cmd_valid=true;
        //emit layoutChanged();
      }
      //sync();
    }break;
    case apc_conf_dsc:
    case apc_conf_read:
    case apc_conf_write: {
      if(!data.size()){
        if(cmd==apc_conf_write)
          save_bkp_timer.start(); //write confirm
        break;
      }
      const int fnum=(unsigned char)data.at(0);
      if(fnum>=fields.size())break;
      fields.at(fnum)->response_received(cmd,data.mid(1));
    }break;
    case apc_script_file:
    case apc_script_read:
    case apc_script_write: {
      foreach(NodesItemField *f,fields){
        if(f->ftype==ft_script)
          f->response_received(cmd,data);
      }
    }break;
    default:
      firmwareLoader.response_received(cmd,data);
      if(blackboxDownload){
        blackboxDownload->response_received(cmd,data);
      }
      return;
  }
  //save backup once everything read and valid
  bool valid=isValid();
  if(valid_state_s!=valid){
    valid_state_s=valid;
    if(valid){
      if(node_info.flags.conf_reset)restoreRecentBackup();
      save_bkp_timer.start();
    }
  }
}
//=============================================================================
void NodesItemNode::chk_inf()
{
  emit request(apc_conf_inf,sn,QByteArray(),50);
  sync();
}
//=============================================================================
void NodesItemNode::blackboxDownloadFinished()
{
  blackboxDownload=NULL;
}
//=============================================================================


