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
#include "node.h"
#include "Vehicles.h"
#include "Vehicle.h"
#include "VehicleMandalaFact.h"
//=============================================================================
NodeField::NodeField(NodeItem *node, quint16 id)
  : Fact(node->f_fields,"field#","","",FactItem,NoData),
    id(id),
    node(node),
    ftype(-1),
    m_valid(false),
    m_dataValid(false),
    m_array(0)
{
  setSection(node->f_fields->title());
  //node->request(apc_conf_dsc,QByteArray().append((unsigned char)id),node->timeout_ms,false);
}
//child of expanded field
NodeField::NodeField(NodeItem *node,NodeField *parent, const QString &name, const QString &title, const QString &descr,int ftype)
  : Fact(parent,name,title,descr,FactItem,NoData),
    id(parent->id),
    node(node),
    ftype(ftype),
    m_valid(true),
    m_dataValid(false),
    m_array(0)
{
  updateDataType();
}
//=============================================================================
bool NodeField::valid() const
{
  return m_valid;
}
void NodeField::setValid(const bool &v)
{
  if(m_valid==v)return;
  m_valid=v;
  emit validChanged();
}
bool NodeField::dataValid() const
{
  return m_dataValid;
}
void NodeField::setDataValid(const bool &v)
{
  if(m_dataValid==v)return;
  m_dataValid=v;
  emit dataValidChanged();
}
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
//=============================================================================
bool NodeField::unpackService(uint ncmd, const QByteArray &data)
{
  switch(ncmd){
    case apc_conf_dsc: {
      if(node->valid() || valid())return true;
      if(!data.size())return true;
      int cnt=data.size();
      const char *str=(const char*)(data.data());
      int sz;
      _node_ft r_ftype=(_node_ft)*str++;
      cnt--;
      sz=strlen(str)+1;
      if(sz>cnt)break;
      const QString r_name(QByteArray(str,sz-1));
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
      //ftype=r_ftype;
      //ba_conf=QByteArray(getConfSize(),0);
      //ba_conf_bkp=QByteArray(getConfSize(),0);
      //conf_descr=r_descr;
      ftype=r_ftype;
      setName(r_name);
      setTitle(r_name);
      setDescr(r_descr);
      setEnumStrings(r_opts);
      updateDataType();
      createSubFields();
      setValid(true);
      node->request(apc_conf_read,QByteArray().append((unsigned char)id),node->timeout_ms,false);
      //check all node fields validity
      bool ok=true;
      foreach (NodeField *f, node->allFields) {
        if(f->valid())continue;
        ok=false;
        break;
      }
      if(!ok)return true;
      //all fields downloaded
      node->setValid(true);
      node->groupFields();
      //qDebug()<<"fields downloaded"<<node->f_fields->size();
    }return true;
    case apc_conf_read: {
      if(!data.size())return true; //requests
      if(!valid())break;
      if(dataValid())return true;
      //qDebug()<<name()<<data.size();
      int sz=ftypeSize();
      if(!unpackValue(data))break;
      setDataValid(true);
      //check all node fields data validity
      if(!node->dataValid()){
        bool ok=true;
        foreach (NodeField *f, node->allFields) {
          if(f->valid())continue;
          ok=false;
          break;
        }
        if(ok)node->setDataValid(true);
      }
      if(data.size()==sz)return true;
      const int fnum=(unsigned char)data.at(sz);
      if(fnum>=node->allFields.size())break;
      return node->allFields.at(fnum)->unpackService(ncmd,data.mid(sz+1));
    }return true;
  }
  //error
  return false;
}
//=============================================================================
bool NodeField::unpackValue(const QByteArray &data)
{
  if(size()){
    QByteArray ba(data);
    foreach (FactTree *i, childItemsTree()) {
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
    //case ft_script: return script?script->getSource():QVariant();break;
  }
  setValue(v);
  return true;
}
//=============================================================================
//=============================================================================
void NodeField::createSubFields(void)
{
  //name truncate
  if(title().contains("_")){
    setTitle(title().remove(0,title().indexOf('_')+1));
  }
  //check for array
  bool force_array=false;
  if(name().contains("[")){
    setArray(name().section("[",1,1).section("]",0,0).toInt());
    setName(name().left(name().indexOf('[')));
    //setTitle(title().left(title().indexOf('[')));
    force_array=true;
  }
  if((array()>1||force_array)){
    //create array items
    QStringList st;
    if(enumStrings().size()==array() && ftype!=ft_option)st=enumStrings();
    for(int i=0;i<array();i++){
      NodeField *fi=new NodeField(node,this,"item#",QString("%1[%2]").arg(name()).arg(i),"",ftype);
      if(st.size())fi->setEnumStrings(st);
    }
    setDataType(ConstData);
    setTreeItemType(GroupItem);
  }
  //special expandable field types, non-arrays
  if(array()<=0){
    switch(ftype){
      case ft_vec:
        if(enumStrings().size()==3){
          foreach(QString s,enumStrings()){
            //new NodesItemField(this,node,s,descr+" ("+s+")",ft_float,QStringList());
            new NodeField(node,this,s,s,descr()+" ("+s+")",ft_float);
          }
          setTreeItemType(GroupItem);
        }
      break;
      case ft_regPID:
        new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
        new NodeField(node,this,"Lp","",tr("Proportional limit")+" [%]",ft_byte);
        new NodeField(node,this,"Kd","",tr("Derivative")+" [K]",ft_float);
        new NodeField(node,this,"Ld","",tr("Derivative limit")+" [%]",ft_byte);
        new NodeField(node,this,"Ki","",tr("Integral")+" [K]",ft_float);
        new NodeField(node,this,"Li","",tr("Integral limit")+" [%]",ft_byte);
        new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
        setTreeItemType(GroupItem);
      break;
      case ft_regPI:
        new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
        new NodeField(node,this,"Lp","",tr("Proportional limit")+" [%]",ft_byte);
        new NodeField(node,this,"Ki","",tr("Integral")+" [K]",ft_float);
        new NodeField(node,this,"Li","",tr("Integral limit")+" [%]",ft_byte);
        new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
        setTreeItemType(GroupItem);
      break;
      case ft_regP:
        new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
        new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
        setTreeItemType(GroupItem);
      break;
      case ft_regPPI:
        new NodeField(node,this,"Kpp","",tr("Error to speed")+" [K]",ft_float);
        new NodeField(node,this,"Lpp","",tr("Speed limit")+" [1/s]",ft_byte);
        new NodeField(node,this,"Kp","",tr("Proportional")+" [K]",ft_float);
        new NodeField(node,this,"Lp","",tr("Proportional limit")+" [%]",ft_byte);
        new NodeField(node,this,"Ki","",tr("Integral")+" [K]",ft_float);
        new NodeField(node,this,"Li","",tr("Integral limit")+" [%]",ft_byte);
        new NodeField(node,this,"Lo","",tr("Output limit")+" [%]",ft_byte);
        setTreeItemType(GroupItem);
      break;
    }
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
//=============================================================================
void NodeField::updateDataType()
{
  switch(ftype){
    default: break;
    case ft_option:
      setDataType(EnumData);
    break;
    case ft_string:
    case ft_lstr:
      setDataType(TextData);
    break;
    case ft_float:
      setDataType(FloatData);
    break;
    case ft_uint:
    case ft_byte:
      setDataType(IntData);
    break;
    case ft_varmsk:{
      QStringList st;
      QList<int> vlist;
      foreach (VehicleMandalaFact *mf, Vehicles::instance()->f_local->f_mandala->allFacts) {
        st.append(mf->name());
        vlist.append(mf->id());
      }
      setEnumStrings(st,vlist);
      setDataType(IntData);
    }break;
  }
}
//=============================================================================
//=============================================================================
