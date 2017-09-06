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
#include "NodesItemField.h"
#include "NodesItemNode.h"
#include "NodesItemGroup.h"
#include "QMandala.h"
#include "PawnScript.h"
#include "crc.h"
//=============================================================================
NodesItemField::NodesItemField(NodesItem *parent, NodesItemNode *node,uint fnum)
 : NodesItem(parent,tr("downloading")+"..."),fnum(fnum),ftype(-1),opts(),array(1),dsc_valid(false),data_valid(false),node(node),field(NULL),script(NULL)
{
  item_type=it_field;
  //connect(this,SIGNAL(changed()),node,SIGNAL(changed()));//,Qt::QueuedConnection);
  connect(this,SIGNAL(changed()),node,SLOT(field_changed()),Qt::QueuedConnection);
  connect(this,SIGNAL(changed()),this,SLOT(updateModelData()),Qt::QueuedConnection);
}
NodesItemField::NodesItemField(NodesItemField *field, NodesItemNode *node,QString name, QString descr, uint ftype, QStringList opts)
 : NodesItem(field,name,descr),fnum(field->fnum),ftype(ftype),opts(opts),array(1),dsc_valid(true),data_valid(true),node(node),field(field),script(NULL)
{
  //child field of expandable field type
  item_type=it_fpart;
  connect(this,SIGNAL(changed()),field,SIGNAL(changed()),Qt::QueuedConnection);
}
NodesItemField::~NodesItemField()
{
  if(script)script->deleteLater();
}
//=============================================================================
void NodesItemField::updateModelData()
{
  node->model->itemDataChanged(this);
}
//=============================================================================
QVariant NodesItemField::data(int column,int role) const
{
  switch(role){
  case Qt::ForegroundRole:
    if(column==tc_field){
      if(!isValid())return QColor(Qt::yellow);
      if(isModified())return QColor(Qt::red).lighter();
      if(fnum==0)return QColor(Qt::gray);
      return QVariant();
    }
    if(column==tc_value){
      if(childItems.size()) return QColor(Qt::darkGray); //expandable
      if(fnum==0)return QColor(Qt::darkYellow);
      if(ftype==ft_string) return QVariant();
      if(ftype==ft_varmsk) return QColor(Qt::cyan);
      return QColor(Qt::cyan).lighter(180);
    }
    return QColor(Qt::darkCyan);
  case Qt::BackgroundRole:
    return QVariant();
  case Qt::FontRole:
    if(column==tc_descr) return QVariant();
    if(ftype>=ft_regPID) return QFont("Monospace",-1,column==tc_field?QFont::Bold:QFont::Normal);
    if(column==tc_field) return QFont("Monospace",-1,QFont::Normal,isModified());
    if(ftype==ft_string) return QFont("",-1,QFont::Normal,true);
    return QVariant();
  case Qt::ToolTipRole:
    if(column==tc_field) return conf_name;
    else if(column==tc_value){
      if(childItems.size()){
        QString s=name;
        foreach(NodesItem *i,childItems)
          s+=QString("\n%1: %2").arg(i->name).arg(i->data(tc_value,Qt::DisplayRole).toString());
        return s;
      }else return data(tc_descr,Qt::DisplayRole);
    }
    return data(column,Qt::DisplayRole);
  }
  //some special types
  if(column==tc_value && (role==Qt::DisplayRole || role==Qt::EditRole)){
    switch(ftype){
      case ft_regPID: return tr("PID");
      case ft_regPI:  return tr("PI");
      case ft_regP:   return tr("P");
      case ft_regPPI: return tr("PPI");
      case ft_vec:    return QVariant(QString().sprintf("%g,%g,%g",((_ft_vec*)ba_conf.data())->x,((_ft_vec*)ba_conf.data())->y,((_ft_vec*)ba_conf.data())->z));
      case ft_script:
        if(!script)return QVariant();
        if(script->isError())return "<"+tr("error")+">";
        if(script->isEmpty())return "<"+tr("empty")+">";
        return QString("%1 Kb").arg(script->size()/1024.0,0,'f',1);
      case ft_uint:
        if(role!=Qt::DisplayRole)break;
        if(units()=="hex") return QString().sprintf("%.4X",NodesItem::data(column,role).toUInt());
        break;
      //deprecated
      case ft_ctr:
      case ft_pwm:
      case ft_gpio:
        return "<"+tr("data")+">";
    }
    if(childItems.size()) return isZero()?tr("default"):"+";
  }
  //regular value display as is
  return NodesItem::data(column,role);
}
//=============================================================================
QVariant NodesItemField::getValue(void) const
{
  const void *ptr=ba_conf.data();
  if(field==parentItem){ //part of expandable field type (vector)
    uint sz=0;
    for(int i=0;i<row();i++)
      sz+=field->childItems.at(i)->getConfSize();
    ptr=field->ba_conf.data()+sz;
  }
  if(!ptr)return "???";
  if(array>1 && childItems.size())return isZero()?"":"+"; //expandable array
  switch(ftype){
    case ft_uint:   return QVariant((uint)*(_ft_uint*)ptr);
    case ft_float:  return QVariant(QString::number(*(_ft_float*)ptr));
    case ft_byte:   return QVariant((uint)*(_ft_byte*)ptr);
    case ft_string: return QVariant(QString(QByteArray((const char*)*(_ft_string*)ptr,sizeof(_ft_string))));
    case ft_lstr:   return QVariant(QString(QByteArray((const char*)*(_ft_lstr*)ptr,sizeof(_ft_lstr))));
    case ft_varmsk: return node->model->mvar->field(*(_ft_varmsk*)ptr)->name();
    case ft_option: return (opts.size())?(*(_ft_option*)ptr<opts.size()?opts.at(*(_ft_option*)ptr):"???"):QString::number(*(_ft_option*)ptr);
    case ft_script: return script?script->getSource():QVariant();
  }
  return ba_conf;
}
//=============================================================================
bool NodesItemField::setValue(const QVariant &value)
{
  void *ptr=ba_conf.data();
  if(field==parentItem){ //part of expandable field type (vector)
    uint sz=0;
    for(int i=0;i<row();i++)
      sz+=field->childItems.at(i)->getConfSize();
    ptr=field->ba_conf.data()+sz;
  }
  if(!ptr)return false;
  bool rv=true;
  switch(ftype){
    case ft_uint:   *(_ft_uint*)ptr=value.toUInt();break;
    case ft_float:  *(_ft_float*)ptr=value.toFloat();break;
    case ft_byte:   *(_ft_byte*)ptr=value.toUInt();break;
    case ft_string: strncpy((char*)*(_ft_string*)ptr,value.toString().toUtf8().data(),sizeof(_ft_string));break;
    case ft_lstr:   strncpy((char*)*(_ft_lstr*)ptr,value.toString().toUtf8().data(),sizeof(_ft_lstr));break;
    case ft_varmsk: *(_ft_varmsk*)ptr=node->model->mvar->field(value.toString())->varmsk();break;
    case ft_vec: {
      QStringList st(value.toString().split(','));
      if(st.size()!=3)return false;
      (*(_ft_vec*)ptr).x=st.at(0).toFloat();
      (*(_ft_vec*)ptr).y=st.at(1).toFloat();
      (*(_ft_vec*)ptr).z=st.at(2).toFloat();
      }
    break;
    case ft_option:
      if(opts.contains(value.toString())) *(_ft_option*)ptr=opts.indexOf(value.toString());
      else if(value.canConvert(QMetaType::UInt) && (int)value.toUInt()<=opts.size()) *(_ft_option*)ptr=value.toUInt();
      else{
        //qWarning("Conf option not accepted: %s: %s",name.toUtf8().data(),value.toString().toUtf8().data());
        return false;
      }
    break;
    case ft_script:
      if(script) rv=script->setSource(value.toString().toUtf8());
    break;
    default:{
      if(value.type()==QVariant::ByteArray){
        QByteArray ba=value.toByteArray();
        if(ba_conf.size()!=ba.size()) return false;
        ba_conf=ba;
        break;
      }
      //try hex array
      QByteArray ba=QByteArray::fromHex(value.toString().toUtf8());
      if(ba_conf.size()!=ba.size()) return false;
      ba_conf=ba;
      break;
    }
  }
  emit changed();
  return rv;
}
//=============================================================================
const QByteArray & NodesItemField::data() const
{
  if(ftype==ft_script && script) return script->data();
  return ba_conf;
}
bool NodesItemField::isValid(void) const
{
  bool rv=field?field->isValid():(dsc_valid&&data_valid);
  if(ftype==ft_script && script) rv&=script->isValid();
  return rv;
}
bool NodesItemField::isModified(void) const
{
  if(!isValid())return false;
  if(field==parentItem){ //part of expandable field type (vector)
    uint sz=0;
    for(int i=0;i<row();i++)
      sz+=field->childItems.at(i)->getConfSize();
    uint ln=getConfSize();
    return memcmp(field->ba_conf.data()+sz,field->ba_conf_bkp.data()+sz,ln)!=0;
  }
  if(ftype==ft_script) return script?script->isModified():false;
  if(ba_conf.size()!=ba_conf_bkp.size())return true;
  return memcmp(ba_conf.data(),ba_conf_bkp.data(),ba_conf.size())!=0;
}
bool NodesItemField::isUpgrading(void) const
{
  return node->isUpgrading();
}
//=============================================================================
Qt::ItemFlags NodesItemField::flags(int column) const
{
  Qt::ItemFlags f=NodesItem::flags(column);
  if(column==tc_value && !childItems.size())
    return f|Qt::ItemIsEditable;
  return f;
}
//=============================================================================
void NodesItemField::invalidate(void)
{
  data_valid=false;
  if(script)script->invalidate();
  emit changed();
}
//=============================================================================
void NodesItemField::backup(void)
{
  ba_conf_bkp=ba_conf;
  if(ftype==ft_script && script)
    script->backup();
  emit changed();
}
void NodesItemField::restore(void)
{
  if(field==parentItem){ //part of expandable field type (vector)
    field->restore();
  }else{
    ba_conf=ba_conf_bkp;
    if(ftype==ft_script && script)
      script->restore();
  }
  emit changed();
}
//=============================================================================
bool NodesItemField::isZero(void) const
{
  if(field==parentItem){ //part of expandable field type (vector)
    uint sz=0;
    for(int i=0;i<row();i++)
      sz+=field->childItems.at(i)->getConfSize();
    uint8_t *ptr=(uint8_t*)field->ba_conf.data()+sz;
    sz=getConfSize();
    while(sz--)
      if(*ptr++!=0)return false;
    return true;
  }
  if(ftype==ft_script)
    return script?script->isEmpty():true;
  for(int i=0;i<ba_conf.size();i++)
    if(ba_conf.at(i)!=0)return false;
  return true;
}
//=============================================================================
int NodesItemField::getConfSize(void) const
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
  return sz*array;
}
QString NodesItemField::typeToString(int t) const
{
  switch(t){
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
QString NodesItemField::units(void) const
{
  if(!descr.contains('['))return QString();
  QString s=descr.mid(descr.lastIndexOf('[')+1);
  s=s.left(s.indexOf(']'));
  if(s.contains(".."))return QString();
  return s;
}
//=============================================================================
void NodesItemField::saveToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("field"));
  dom.toElement().setAttribute("idx",QString::number(fnum));
  dom.toElement().setAttribute("name",conf_name);

  QDomNode e=dom.appendChild(doc.createElement("struct"));
  e.appendChild(doc.createElement("type")).appendChild(doc.createTextNode(typeToString(ftype)));
  if(descr.size())e.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(conf_descr));
  if(opts.size())e.appendChild(doc.createElement("opts")).appendChild(doc.createTextNode(opts.join(',')));
  saveValueToXml(dom);
}
//=============================================================================
void NodesItemField::saveValueToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  if(childCount()){
    foreach(NodesItem *i,childItems) {
      QDomNode e=dom.appendChild(doc.createElement("value"));
      e.toElement().setAttribute("idx",QString::number(i->row()));
      e.toElement().setAttribute("name",i->name);
      e.appendChild(doc.createTextNode(i->getValue().toString()));
    }
  }else{
    if(!saveOldValueToXml(dom))
      dom.appendChild(doc.createElement("value")).appendChild(doc.createTextNode(getValue().toString()));
  }
}
//=============================================================================
void NodesItemField::loadFromXml(QDomNode dom)
{
  //dom is "field" tag, with idx==fnum
  bool bLoadStruct=!isValid();
  if(bLoadStruct){
    QString r_name=dom.toElement().attribute("name");
    clear();
    name=r_name;
    conf_name=r_name;
    data_valid=false;
    //load field struct
    QDomElement e=dom.firstChildElement("struct");
    if(!e.isNull()){
      QString stype=e.firstChildElement("type").text();
      for(int i=0;i<ft_cnt;i++){
        QString s=typeToString(i);
        if(!s.size())break;
        if(s!=stype)continue;
        ftype=i;
        break;
      }
      descr=e.firstChildElement("descr").text();
      conf_descr=descr;
      opts=e.firstChildElement("opts").text().split(',',QString::SkipEmptyParts);
      if(ftype<ft_cnt && name.size()){
        dsc_valid=data_valid=true;
        ba_conf=QByteArray(getConfSize(),0);
        ba_conf_bkp=QByteArray(getConfSize(),0);
        arrange();
        if(script)script->validate();
        //qDebug()<<name;
      }
    }
  }
  if(!dsc_valid)return;
  //value
  QDomElement e=dom.firstChildElement("value");
  while(!e.isNull()){
    if(childCount()){
      int idx=e.attribute("idx").toUInt();
      if(idx<childCount()) childItems.at(idx)->setValue(e.text());
    }else{
      if(!(name=="comment" && e.text().isEmpty()))
        setValue(e.text());
    }
    if(bLoadStruct){
      ba_conf_bkp=ba_conf;
      data_valid=true;
      if(script)script->validate();
    }
    e=e.nextSiblingElement(e.tagName());
  }
}
//=============================================================================
void NodesItemField::sync(void)
{
  if(!dsc_valid){
    emit request(apc_conf_dsc,node->sn,QByteArray().append((unsigned char)fnum),500);
    return;
  }
}
//=============================================================================
void NodesItemField::upload(void)
{
  if(!isValid()){
    sync();
    return;
  }
  if(!isModified())return;

  if(ftype==ft_script){
    if(script)script->upload();
  }else{
    emit request(apc_conf_write,node->sn,QByteArray().append((unsigned char)fnum).append(ba_conf),1000);
  }

  //save to telemetry
  QDomDocument doc;
  QDomNode dom=doc.appendChild(doc.createElement("node_conf"));
  dom.toElement().setAttribute("sn",QString(node->sn.toHex().toUpper()));
  dom.toElement().setAttribute("name",node->name);
  dom.toElement().setAttribute("f",conf_name);
  saveValueToXml(dom);
  QBuffer b;
  b.open(QBuffer::WriteOnly);
  QTextStream stream(&b);
  stream<<"\n";
  doc.save(stream,2);
  QByteArray ba(b.data());
  if(ba.endsWith('\n'))ba.resize(ba.size()-1);
  node->model->mvar->rec->record_xml(ba);
}
//=============================================================================
void NodesItemField::response_received(unsigned char cmd,const QByteArray data)
{
  switch(cmd){
    case apc_conf_dsc: {
      //qDebug("apc_conf_dsc (num: %u, cnt:%u)",fnum,data.size());
      if(dsc_valid)break;
      int cnt=data.size();
      const char *str=data.data();
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
      dsc_valid=true;
      ftype=r_ftype;
      ba_conf=QByteArray(getConfSize(),0);
      ba_conf_bkp=QByteArray(getConfSize(),0);
      descr=r_descr;
      opts=r_opts;
      name=r_name;
      conf_name=r_name;
      conf_descr=r_descr;
      arrange();
      sync();
    }break;
    case apc_conf_read: {
      //qDebug("apc_conf_read (num: %u, cnt:%u)",fnum,data.size());
      if(!dsc_valid)break;
      if(!data.size())break; //requests
      if(isValid())break;
      uint sz=getConfSize();
      uint cnt=data.size();
      if(cnt<sz){
        qWarning("Wrong node conf size from '%s' (num: %u exp: %u re: %u)",node->model->mvar->node_name(node->sn),fnum,getConfSize(),data.size());
        break;
      }else if(cnt>sz){
        //qDebug("Field burst: %u",cnt);
        ba_conf_bkp=ba_conf=data.left(sz);
        data_valid=true;
        const int fnum=(unsigned char)data.at(sz);
        if(fnum>=node->fields.size())break;
        node->fields.at(fnum)->response_received(cmd,data.mid(sz+1));
        //response_received(cmd,data.left(sz));
        emit changed();
        break;
      }
      ba_conf_bkp=ba_conf=data;
      data_valid=true;
      emit changed();
      node->sync();
    }break;
    case apc_conf_write:
      //qDebug("apc_conf_write field (cnt:%u)",data.size());
      if(!dsc_valid)break;
      if(data.size()){
        //uplink write (player)
        if(data.size()!=ba_conf.size())break;
        ba_conf=data;
        data_valid=true;
        emit changed();
        qDebug("%s: %s/%s=%s",tr("Field modified").toUtf8().data(),node->name.toUtf8().data(),name.toUtf8().data(),this->data(tc_value,Qt::DisplayRole).toString().toUtf8().data());
        break;
      }
      //zero data size - write confirmation
      if(!data_valid)break;
      ba_conf_bkp=ba_conf;
      emit changed();
      break;
  }
  //forward to special fmt if any
  if(dsc_valid && data_valid && ftype==ft_script && script){
    script->response_received(cmd,data);
  }
}
//=============================================================================
void NodesItemField::arrange()
{
  if(ftype==ft_script && script==NULL)
    script=new PawnScript(this);
  //name truncate
  if(name.contains("_")){
    name.remove(0,name.indexOf('_')+1);
  }
  //grouping
  while(descr.contains(':')){
    QString group=descr.left(descr.indexOf(':'));
    descr=descr.remove(0,descr.indexOf(':')+1).trimmed();
    NodesItem *group_item=find(group,parentItem);
    if((!group_item) || group_item==this)
      group_item=new NodesItemGroup(parentItem,group);
    node->model->beginResetModel();
    parentItem->removeChild(this);
    group_item->appendChild(this);
    node->model->endResetModel();
    if(name.contains('_')&&name.left(name.indexOf('_'))==group)
      name.remove(0,name.indexOf('_')+1);
  }
  //check for array
  bool force_array=false;
  if(name.contains("[")){
    array=name.section("[",1,1).section("]",0,0).toUInt();
    name=name.left(name.indexOf('['));
    ba_conf=QByteArray(getConfSize(),0);
    ba_conf_bkp=QByteArray(getConfSize(),0);
    force_array=true;
  }
  if((array>1||force_array)&&(ftype!=ft_pwm)&&(ftype!=ft_ctr)&&(ftype!=ft_gpio)){
    //create array items
    QStringList st;
    if(opts.size()==(int)array && ftype!=ft_option)st=opts;
    for(uint i=0;i<array;i++){
      new NodesItemField(this,node,(int)i<st.size()?st.at(i):QString::number(i+1),units().size()?QString("[%1]").arg(units()):"",ftype,opts);
    }
  }

  //special expandable field types, non-arrays
  if(array<=1){
    switch(ftype){
      case ft_vec:
        if(opts.size()==3){
          node->model->beginResetModel();
          foreach(QString s,opts){
            new NodesItemField(this,node,s,descr+" ("+s+")",ft_float,QStringList());
          }
          node->model->endResetModel();
        }
      break;
      case ft_regPID:
        node->model->beginResetModel();
        new NodesItemField(this,node,"Kp",tr("Proportional")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Lp",tr("Proportional limit")+" [%]",ft_byte,QStringList());
        new NodesItemField(this,node,"Kd",tr("Derivative")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Ld",tr("Derivative limit")+" [%]",ft_byte,QStringList());
        new NodesItemField(this,node,"Ki",tr("Integral")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Li",tr("Integral limit")+" [%]",ft_byte,QStringList());
        new NodesItemField(this,node,"Lo",tr("Output limit")+" [%]",ft_byte,QStringList());
        node->model->endResetModel();
      break;
      case ft_regPI:
        node->model->beginResetModel();
        new NodesItemField(this,node,"Kp",tr("Proportional")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Lp",tr("Proportional limit")+" [%]",ft_byte,QStringList());
        new NodesItemField(this,node,"Ki",tr("Integral")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Li",tr("Integral limit")+" [%]",ft_byte,QStringList());
        new NodesItemField(this,node,"Lo",tr("Output limit")+" [%]",ft_byte,QStringList());
        node->model->endResetModel();
      break;
      case ft_regP:
        node->model->beginResetModel();
        new NodesItemField(this,node,"Kp",tr("Proportional")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Lo",tr("Output limit")+" [%]",ft_byte,QStringList());
        node->model->endResetModel();
      break;
      case ft_regPPI:
        node->model->beginResetModel();
        new NodesItemField(this,node,"Kpp",tr("Error to speed")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Lpp",tr("Speed limit")+" [m/s]",ft_byte,QStringList());
        new NodesItemField(this,node,"Kp",tr("Proportional")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Lp",tr("Proportional limit")+" [%]",ft_byte,QStringList());
        new NodesItemField(this,node,"Ki",tr("Integral")+" [K]",ft_float,QStringList());
        new NodesItemField(this,node,"Li",tr("Integral limit")+" [%]",ft_byte,QStringList());
        new NodesItemField(this,node,"Lo",tr("Output limit")+" [%]",ft_byte,QStringList());
        node->model->endResetModel();
      break;
    }
  }
  //emit changed();
}
//=============================================================================
bool NodesItemField::saveOldValueToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  switch(ftype){
    case ft_ctr:{
        _ft_ctr * p=(_ft_ctr*)ba_conf.data();
        for(uint i=0;i<array;i++){
          QString vn=node->model->mvar->field(p[i].varmsk)->name();
          QDomNode e=dom.appendChild(doc.createElement("value"));
          e.toElement().setAttribute("idx",QString::number(i));
          e.appendChild(doc.createElement("var")).appendChild(doc.createTextNode(vn));
          e.appendChild(doc.createElement("mult")).appendChild(doc.createTextNode(QString::number(p[i].mult)));
          e.appendChild(doc.createElement("diff")).appendChild(doc.createTextNode(QString::number(p[i].diff)));
          e.appendChild(doc.createElement("speed")).appendChild(doc.createTextNode(QString::number(p[i].speed)));
          e.appendChild(doc.createElement("pwm_ch")).appendChild(doc.createTextNode(QString::number(p[i].pwm_ch)));
        }
    }return true;
    case ft_pwm:{
        _ft_pwm * p=(_ft_pwm*)ba_conf.data();
        for(uint i=0;i<array;i++){
          QDomNode e=dom.appendChild(doc.createElement("value"));
          e.toElement().setAttribute("idx",QString::number(i));
          e.appendChild(doc.createElement("zero")).appendChild(doc.createTextNode(QString::number(p[i].zero)));
          e.appendChild(doc.createElement("max")).appendChild(doc.createTextNode(QString::number(p[i].max)));
          e.appendChild(doc.createElement("min")).appendChild(doc.createTextNode(QString::number(p[i].min)));
        }
    }return true;
    case ft_gpio:{
        _ft_gpio * p=(_ft_gpio*)ba_conf.data();
        for(uint i=0;i<array;i++){
          QString vn=node->model->mvar->field(p[i].varmsk)->name();
          QDomNode e=dom.appendChild(doc.createElement("value"));
          e.toElement().setAttribute("idx",QString::number(i));
          e.appendChild(doc.createElement("var")).appendChild(doc.createTextNode(vn));
          e.appendChild(doc.createElement("opt")).appendChild(doc.createTextNode(QString::number(p[i].opt)));
          e.appendChild(doc.createElement("mult")).appendChild(doc.createTextNode(QString::number(p[i].mult)));
        }
    }return true;
  }
  return false;
}
//=============================================================================




