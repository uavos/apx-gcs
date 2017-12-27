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
#include "NodeField.h"
#include "NodeItem.h"
#include "Nodes.h"
#include <node.h>
#include <Vehicles.h>
#include <Vehicle.h>
#include <VehicleMandalaFact.h>
#include "PawnScript.h"
//=============================================================================
NodeField::NodeField(NodeItem *node, quint16 id)
  : NodeFieldBase(NULL,"field#","","",FactItem,NoData),
    id(id),
    ftype(-1),
    node(node),
    script(NULL),
    parentField(NULL),
    m_array(0)
{
  connect(this,&NodeField::dictValidChanged,this,&NodeField::validateDict);
  connect(this,&NodeField::dataValidChanged,this,&NodeField::validateData);

}
//child of expanded field
NodeField::NodeField(NodeItem *node,NodeField *parent, const QString &name, const QString &title, const QString &descr,int ftype)
  : NodeFieldBase(parent,name,title,descr,FactItem,NoData),
    id(parent->id),
    ftype(ftype),
    node(node),
    script(NULL),
    parentField(parent),
    m_array(0)
{
  //connect(this,&NodeField::dataValidChanged,this,&NodeField::validateData);
  updateDataType();
  connect(this,&NodeField::valueChanged,parent,&NodeField::updateStatus);
  parent->updateStatus();
}
//=============================================================================
int NodeField::array() const
{
  return m_array;
}
void NodeField::setArray(const int &v)
{
  if(m_array==v)return;
  m_array=v;
  emit arrayChanged();
}
//=============================================================================
void NodeField::updateStatus()
{
  if(ftype==ft_vec){
    QStringList st;
    foreach (FactTree *i, childItems()) {
      NodeField *f=static_cast<NodeField*>(i);
      st.append(f->text());
    }
    setStatus(QString("(%1)").arg(st.join(',')));
  }else if(array()>0){
    int acnt=0;
    foreach (FactTree *i, childItems()) {
      NodeField *f=static_cast<NodeField*>(i);
      if(f->isZero())continue;
      QString s=f->text();
      if(s.isEmpty())continue;
      if(s=="0")continue;
      acnt++;
    }
    if(acnt>0)setStatus(QString("[%1/%2]").arg(acnt).arg(size()));
    else setStatus(QString("[%1]").arg(size()));
  }else if(ftype==ft_script){
    if(script->error())setStatus("<"+tr("error")+">");
    else if(value().toString().isEmpty())setStatus("<"+tr("empty")+">");
    else setStatus(QString("%1 Kb").arg(script->size()/1024.0,0,'f',1));
  }
}
//=============================================================================
void NodeField::setModified(const bool &v)
{
  if(m_modified==v)return;
  FactData::setModified(v);
  if(v){
    //qDebug()<<"mod"<<path();
    //set all parents to modified=true
    for(FactTree *i=parentItem();i!=node->nodes->parentItem();i=i->parentItem()){
      Fact *f=qobject_cast<Fact*>(i);
      if(f)f->setModified(v);
      else break;
    }
    return;
  }
  //refresh modified status of all parent items
  for(FactTree *i=parentItem();i && i!=node->nodes->parentItem();i=i->parentItem()){
    foreach (FactTree *c, i->childItems()) {
      Fact *f=qobject_cast<Fact*>(c);
      if(f){
        if(f->modified())return;
      }else break;
    }
    static_cast<Fact*>(i)->setModified(v);
  }
}
//=============================================================================
//=============================================================================
QString NodeField::mandalaToString(quint16 mid) const
{
  VehicleMandalaFact *mf=node->nodes->vehicle->f_mandala->factById(mid);
  return mf?mf->title():QString();
}
quint16 NodeField::stringToMandala(const QString &s) const
{
  if((!s.isEmpty()) && s!="0"){
    VehicleMandalaFact *mf=node->nodes->vehicle->f_mandala->factByName(s);
    if(mf)return mf->id();
    //try int
    bool ok=false;
    int i=s.toInt(&ok);
    if(ok){
      mf=node->nodes->vehicle->f_mandala->factById(i);
      if(mf)return mf->id();
    }
  }
  return 0;
}
const QStringList * NodeField::mandalaNames() const
{
  return & node->nodes->vehicle->f_mandala->names;
}
//=============================================================================
//=============================================================================
bool NodeField::unpackService(uint ncmd, const QByteArray &data)
{
  switch(ncmd){
    case apc_conf_dsc: {
      if(dictValid())return true;
      if(!data.size())return true;
      //qDebug()<<"apc_conf_dsc"<<data.size();
      int cnt=data.size();
      const char *str=(const char*)(data.data());
      int sz;
      _node_ft r_ftype=(_node_ft)*str++;
      cnt--;
      sz=strlen(str)+1;
      if(sz>cnt)break;
      QString r_name(QByteArray(str,sz-1));
      str+=sz;
      cnt-=sz;
      sz=strlen(str)+1;
      if(sz>cnt)break;
      QString r_descr(QByteArray(str,sz-1));
      str+=sz;
      cnt-=sz;
      sz=strlen(str)+1;
      if(sz>cnt)break;
      QStringList r_opts(QString(QByteArray(str,sz-1)).split(',',QString::SkipEmptyParts));
      str+=sz;
      cnt-=sz;
      //extract opts from descr if any
      if(r_opts.size()==0 && r_descr.contains('(')){
        QString s(r_descr.mid(r_descr.indexOf('(')+1).trimmed());
        r_opts=QStringList(s.left(s.lastIndexOf(')')).split(','));
        if(r_opts.size()<2)r_opts.clear();
        else r_descr=r_descr.left(r_descr.indexOf('(')).trimmed();
      }
      if(r_opts.size()==1)r_opts.clear();
      //truncate opts by '_'
      if(r_opts.size()&&r_opts.first().contains("_")){
        QStringList nopts;
        foreach(QString opt,r_opts)
          nopts.append(opt.mid(opt.lastIndexOf('_')+1));
        r_opts=nopts;
      }
      //default opts
      if((r_ftype==ft_option)&&(!r_opts.size()))
        r_opts=QStringList()<<"no"<<"yes";
      if(cnt!=0){
        qWarning("Error node_conf descriptor received (cnt:%u)",cnt);
        break;
      }
      ftype=r_ftype;
      //preprocess expand
      if(r_name.contains("[")){
        setArray(r_name.section("[",1,1).section("]",0,0).toInt());
        r_name=r_name.left(r_name.indexOf('['));
      }
      setName(r_name);
      //trim title
      if(r_name.contains("_")){
        r_name.remove(0,r_name.indexOf('_')+1);
      }
      //units from descr
      setUnits("");
      if(r_descr.contains('[')){
        QString s=r_descr.mid(r_descr.lastIndexOf('[')+1);
        r_descr=r_descr.left(r_descr.indexOf('['));
        s=s.left(s.indexOf(']'));
        if(!s.contains("..")) setUnits(s);
      }
      //groups from descr
      groups.clear();
      while(r_descr.contains(':')){
        QString s=r_descr.left(r_descr.indexOf(':')).trimmed();
        groups.append(s);
        r_descr=r_descr.remove(0,r_descr.indexOf(':')+1).trimmed();
        if(r_name.contains('_') && r_name.left(r_name.indexOf('_')).toLower()==s.toLower())
          r_name.remove(0,r_name.indexOf('_')+1);
      }
      setTitle(r_name);
      setDescr(r_descr);
      setEnumStrings(r_opts);
      setDictValid(true);
      /*if(node->dictValid())
        node->nodes->db->nodeDictWrite(node);*/
      //qDebug()<<"fields downloaded"<<node->allFields.size();
    }return true;
    case apc_conf_read: {
      //qDebug()<<path();
      if(!data.size())return true; //requests
      if(!dictValid())break;
      int sz=ftypeSize();
      if(!dataValid()){
        //qDebug()<<name()<<data.size();
        if(!unpackValue(data))break;
        if(ftype!=ft_script){
          setDataValid(true);
        }
        if(data.size()==sz){
          foreach (NodeField *f, node->allFields) {
            if(f->dataValid())continue;
            if(f->script && f->script->isBusy())continue;
            node->request(apc_conf_read,QByteArray().append((unsigned char)f->id),node->timeout_ms,false);
            break;
          }
          return true;
        }
      }
      if(data.size()>sz){
        //continue bulk unpack
        const int fnum=(unsigned char)data.at(sz);
        if(fnum>=node->allFields.size())break;
        return node->allFields.at(fnum)->unpackService(ncmd,data.mid(sz+1));
      }
    }return true;
    case apc_conf_write: {
      if(!dictValid())break;
      if(!dataValid())break;
      if(data.size()){
        //uplink write (player)
        if(data.size()!=ftypeSize())break;
        unpackValue(data);
        node->message(QString("%1: %2=%3").arg(tr("Field modified")).arg(title()).arg(text()));
        return true;
      }
      //zero data size - update confirmation
      backup();
    }return true;
    case apc_script_file:
    case apc_script_read:
    case apc_script_write:
      if(script)script->unpackService(ncmd,data);
      return true;
  }
  //error
  return false;
}
//=============================================================================
bool NodeField::unpackValue(const QByteArray &data)
{
  if(size()){
    QByteArray ba(data);
    foreach (FactTree *i, childItems()) {
      NodeField *f=static_cast<NodeField*>(i);
      if(!f->unpackValue(ba))return false;
      ba=ba.mid(f->ftypeSize());
    }
    return true;
  }
  if(data.size()<ftypeSize())return false;
  const void *ptr=data.data();
  QVariant v;
  switch(ftype){
    case ft_uint:   v=QVariant((quint32)*(_ft_uint*)ptr);break;
    case ft_float:  v=QVariant((qreal)(*(_ft_float*)ptr));break;
    case ft_byte:   v=QVariant((quint8)*(_ft_byte*)ptr);break;
    case ft_string: v=QVariant(QString(QByteArray((const char*)*(_ft_string*)ptr,sizeof(_ft_string))));break;
    case ft_lstr:   v=QVariant(QString(QByteArray((const char*)*(_ft_lstr*)ptr,sizeof(_ft_lstr))));break;
    case ft_varmsk: v=QVariant((quint16)*(_ft_varmsk*)ptr);break; //node->model->mvar->field(*(_ft_varmsk*)ptr)->name();break;
    case ft_option: v=QVariant((quint8)*(_ft_option*)ptr);break; //(opts.size())?(*(_ft_option*)ptr<opts.size()?opts.at(*(_ft_option*)ptr):"???"):QString::number(*(_ft_option*)ptr);break;
    case ft_script: {
      if(!script) break;
      _ft_script *scr=(_ft_script*)ptr;
      //qDebug()<<"scr:"<<scr->code_size<<scr->size<<path();
      if(!scr->size){
        script->unpackFlashData();
      }else{
        script->download();
      }
      return true;
    }break;
  }
  setValue(v);
  return true;
}
//=============================================================================
QByteArray NodeField::packValue() const
{
  QByteArray ba;
  if(size()){
    foreach (FactTree *i, childItems()) {
      NodeField *f=static_cast<NodeField*>(i);
      ba.append(f->packValue());
    }
    return ba;
  }
  ba.resize(ftypeSize());
  void *ptr=ba.data();

  switch(ftype){
    case ft_uint:   *(_ft_uint*)ptr=value().toUInt();break;
    case ft_float:  *(_ft_float*)ptr=value().toFloat();break;
    case ft_byte:   *(_ft_byte*)ptr=value().toUInt();break;
    case ft_string: strncpy((char*)*(_ft_string*)ptr,value().toString().toUtf8().data(),sizeof(_ft_string));break;
    case ft_lstr:   strncpy((char*)*(_ft_lstr*)ptr,value().toString().toUtf8().data(),sizeof(_ft_lstr));break;
    case ft_varmsk: *(_ft_varmsk*)ptr=value().toUInt();break;
    case ft_option: *(_ft_option*)ptr=value().toUInt();break;
    //case ft_script: if(script) return script->setSource(value.toString().toUtf8());break;
  }
  return ba;
}
//=============================================================================
//=============================================================================
void NodeField::validateDict()
{
  if(!dictValid())return;
  updateDataType();
  node->validateDict();
}
//=============================================================================
void NodeField::validateData()
{
  if(!dataValid())return;
  backup();
  //node->nodes->db->nodeDataWrite(this);
  //check all node fields data validity
  if(!node->dataValid()){
    bool ok=true;
    foreach (NodeField *f, node->allFields) {
      if(f->dataValid())continue;
      ok=false;
      break;
    }
    if(ok)node->setDataValid(true);
  }
  //qDebug()<<"dataValid"<<path();
}
//=============================================================================
void NodeField::createSubFields(void)
{
  if(size()>0)return; //already created

  //check if comment field and bind to node value
  if(id==0 && name()=="comment"){
    node->setStatus(text());
    connect(this,&NodeField::textChanged,this,[=](){node->setStatus(text());});
  }

  if(array()>1){
    //fact is array
    QStringList stNames;
    if(name()=="ctr_ch" && array()>0){
      QStringList st;
      int sz=0;
      foreach (NodeField *f, node->allFields) {
        if(!f->name().startsWith("ctr_ch_"))continue;
        f->createSubFields();
        sz=f->array();
        break;
      }
      for(int i=0;i<sz;i++) st<<QString("CH%1").arg(i+1);
      setEnumStrings(st);
      setDataType(EnumData);
    }else if(m_enumStrings.size()==array() && ftype!=ft_option){
      stNames=m_enumStrings;
    }
    //create array items
    for(int i=0;i<array();i++){
      const QString stitle=stNames.value(i,QString::number(i+1));
      NodeField *fi=new NodeField(node,this,QString("%1_%2").arg(name()).arg(i+1),stitle,QString("%1/%2").arg(title()).arg(stitle),ftype);
      fi->setUnits(units());
      if(stNames.isEmpty() && (!m_enumStrings.isEmpty())){
        fi->setEnumStrings(m_enumStrings);
        fi->setDataType(EnumData);
      }
      //fi->setVisible(false);
    }
    setDataType(NoData);
    setTreeItemType(GroupItem);
  }
}
//=============================================================================
int NodeField::ftypeSize() const
{
  int sz=0;
  switch(ftype){
    case ft_option: sz=sizeof(_ft_option);break;
    case ft_varmsk: sz=sizeof(_ft_varmsk);break;
    case ft_uint:   sz=sizeof(_ft_uint);break;
    case ft_float:  sz=sizeof(_ft_float);break;
    case ft_vec:    sz=sizeof(_ft_vec);break;
    case ft_ctr:    sz=sizeof(_ft_ctr);break;
    case ft_pwm:    sz=sizeof(_ft_pwm);break;
    case ft_gpio:   sz=sizeof(_ft_gpio);break;
    case ft_byte:   sz=sizeof(_ft_byte);break;
    case ft_string: sz=sizeof(_ft_string);break;
    case ft_lstr:   sz=sizeof(_ft_lstr);break;
    case ft_regPID: sz=sizeof(_ft_regPID);break;
    case ft_regPI:  sz=sizeof(_ft_regPI);break;
    case ft_regP:   sz=sizeof(_ft_regP);break;
    case ft_regPPI: sz=sizeof(_ft_regPPI);break;
    case ft_raw:    sz=sizeof(_ft_raw);break;
    case ft_script: sz=sizeof(_ft_script);break;
  }
  if(array()>0)sz*=array();
  return sz;
}
QString NodeField::ftypeString(int i) const
{
  switch(i<0?ftype:i){
    case ft_option: return "option";
    case ft_varmsk: return "varmsk";
    case ft_uint:   return "uint";
    case ft_float:  return "float";
    case ft_vec:    return "vec";
    case ft_ctr:    return "ctr";
    case ft_pwm:    return "pwm";
    case ft_gpio:   return "gpio";
    case ft_byte:   return "byte";
    case ft_string: return "string";
    case ft_lstr:   return "lstr";
    case ft_regPID: return "regPID";
    case ft_regPI:  return "regPI";
    case ft_regP:   return "regP";
    case ft_regPPI: return "regPPI";
    case ft_raw:    return "raw";
    case ft_script: return "script";
  }
  return "";
}
//=============================================================================
void NodeField::updateDataType()
{
  switch(ftype){
    default: break;
    case ft_option:
      setMin(0);
      setDataType(EnumData);
    break;
    case ft_string:
    case ft_lstr:
      setDataType(TextData);
    break;
    case ft_float:
      setPrecision(5);
      setDataType(FloatData);
    break;
    case ft_byte:
      setMax(255);
    case ft_uint:
      setMin(0);
      setDataType(IntData);
    break;
    case ft_varmsk:{
      setDataType(MandalaData);
    }break;
    case ft_script:
      if(script)break;
      script=new PawnScript(this);
      setDataType(ScriptData);
      connect(script,&PawnScript::changed,this,&NodeField::updateStatus);
    break;
    case ft_vec:
      if(enumStrings().size()==3 && size()==0){
        foreach(QString s,enumStrings()){
          //new NodesItemField(this,node,s,descr+" ("+s+")",ft_float,QStringList());
          new NodeField(node,this,s,s,descr()+" ("+s+")",ft_float);
        }
        setDataType(NoData);
        setTreeItemType(GroupItem);
      }
    break;
    case ft_regPID:
      if(size()>0)break;
      new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
      new NodeField(node,this,"Lp","",tr("Proportional limit")+" [%]",ft_byte);
      new NodeField(node,this,"Kd","",tr("Derivative")+" [K]",ft_float);
      new NodeField(node,this,"Ld","",tr("Derivative limit")+" [%]",ft_byte);
      new NodeField(node,this,"Ki","",tr("Integral")+" [K]",ft_float);
      new NodeField(node,this,"Li","",tr("Integral limit")+" [%]",ft_byte);
      new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
      setDataType(NoData);
      setTreeItemType(GroupItem);
    break;
    case ft_regPI:
      if(size()>0)break;
      new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
      new NodeField(node,this,"Lp","",tr("Proportional limit")+" [%]",ft_byte);
      new NodeField(node,this,"Ki","",tr("Integral")+" [K]",ft_float);
      new NodeField(node,this,"Li","",tr("Integral limit")+" [%]",ft_byte);
      new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
      setDataType(NoData);
      setTreeItemType(GroupItem);
    break;
    case ft_regP:
      if(size()>0)break;
      new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
      new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
      setDataType(NoData);
      setTreeItemType(GroupItem);
    break;
    case ft_regPPI:
      if(size()>0)break;
      new NodeField(node,this,"Kpp","",tr("Error to speed")+" [K]",ft_float);
      new NodeField(node,this,"Lpp","",tr("Speed limit")+" [1/s]",ft_byte);
      new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
      new NodeField(node,this,"Lp","",tr("Proportional limit")+" [%]",ft_byte);
      new NodeField(node,this,"Ki","",tr("Integral")+" [K]",ft_float);
      new NodeField(node,this,"Li","",tr("Integral limit")+" [%]",ft_byte);
      new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
      setDataType(NoData);
      setTreeItemType(GroupItem);
    break;
  }
}
//=============================================================================
//=============================================================================
void NodeField::hashData(QCryptographicHash *h) const
{
  Fact::hashData(h);
  h->addData(ftypeString().toUtf8());
  h->addData(QString::number(id).toUtf8());
  h->addData(QString::number(ftype).toUtf8());
}
//=============================================================================
QString NodeField::fpath(const QChar pathDelimiter) const
{
  QStringList st;
  for(FactTree *i=parentItem();i;i=i->parentItem()){
    if(i==node)break;
    Fact *f=static_cast<Fact*>(i);
    st.prepend(f->title());
  }
  return st.join(pathDelimiter);
}
//=============================================================================
