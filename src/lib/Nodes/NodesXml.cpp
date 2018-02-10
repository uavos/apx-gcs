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
#include "NodesXml.h"
#include "Nodes.h"
#include "Vehicle.h"
#include <node.h>
//=============================================================================
NodesXml::NodesXml(Nodes *parent)
  : QObject(parent), nodes(parent), format(1)
{
}
//=============================================================================
void NodesXml::write(QDomNode dom, NodeItem *node) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("node"));
  dom.toElement().setAttribute("sn",QString(node->sn.toHex().toUpper()));
  dom.toElement().setAttribute("name",node->title());

  dom.appendChild(doc.createElement("version")).appendChild(doc.createTextNode(node->version()));
  dom.appendChild(doc.createElement("hardware")).appendChild(doc.createTextNode(node->hardware()));

  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(node->conf_hash));
  dom.appendChild(doc.createElement("date")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));

  if(node->commands.cmd.size()){
    QDomNode e=dom.appendChild(doc.createElement("commands"));
    e.toElement().setAttribute("cnt",QString::number(node->commands.cmd.size()));
    for(int i=0;i<node->commands.cmd.size();i++){
      QDomNode e2=e.appendChild(doc.createElement("command"));
      e2.toElement().setAttribute("id",QString::number(node->commands.cmd.at(i)));
      e2.appendChild(doc.createElement("name")).appendChild(doc.createTextNode(node->commands.name.at(i)));
      e2.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(node->commands.descr.at(i)));
    }
  }

  QDomNode domf=dom.appendChild(doc.createElement("fields"));
  domf.toElement().setAttribute("cnt",QString::number(node->allFields.size()));

  foreach(NodeField *f,node->allFields){
    QDomNode dom=domf.appendChild(doc.createElement("field"));
    dom.toElement().setAttribute("id",QString::number(f->id));
    dom.toElement().setAttribute("name",f->name());
    dom.appendChild(doc.createElement("ftype")).appendChild(doc.createTextNode(f->ftypeString()));
    dom.appendChild(doc.createElement("title")).appendChild(doc.createTextNode(f->title()));
    if(!f->descr().isEmpty())
      dom.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(f->descr()));
    if(!f->units().isEmpty())
      dom.appendChild(doc.createElement("units")).appendChild(doc.createTextNode(f->units()));
    if(!f->defaultValue().isNull())
      dom.appendChild(doc.createElement("defValue")).appendChild(doc.createTextNode(f->defaultValue().toString()));
    if(f->array()>1)
      dom.appendChild(doc.createElement("array")).appendChild(doc.createTextNode(QString::number(f->array())));
    if(!f->enumStrings().isEmpty())
      dom.appendChild(doc.createElement("opts")).appendChild(doc.createTextNode(f->enumStrings().join(',')));
    if(!f->groups.isEmpty())
      dom.appendChild(doc.createElement("sect")).appendChild(doc.createTextNode(f->groups.join('/')));

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
void NodesXml::write(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("nodes"));
  dom.toElement().setAttribute("fmt",QString::number(format));
  dom.toElement().setAttribute("href","http://www.uavos.com/");
  dom.toElement().setAttribute("title",nodes->vehicle->fileTitle());
  dom.toElement().setAttribute("version",FactSystem::version());
  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(QString(nodes->hash().toHex().toUpper())));
  dom.appendChild(doc.createElement("date")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));
  //saveIdentToXml(dom);
  foreach(NodeItem *node,nodes->snMap.values()){
    write(dom,node);
  }
}
//=============================================================================
QDomDocument NodesXml::write() const
{
  QDomDocument doc;
  doc.appendChild(doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\""));
  write(doc);
  return doc;
}
//=============================================================================
//=============================================================================
int NodesXml::read(QDomNode dom) const
{
  int icnt=0;
  if(nodes->snMap.isEmpty()){
    //load and construct nodes from file
    int fmt=dom.toElement().attribute("fmt").toInt();
    dom.childNodes().size();
    for(QDomElement e=dom.firstChildElement("node");!e.isNull();e=e.nextSiblingElement(e.tagName())){
      QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
      QString name=e.attribute("name");
      QDomElement ei=fmt==0?e.firstChildElement("info"):e;
      if(!ei.isNull()){
        NodeItem *node=nodes->nodeCheck(sn);
        node->setName(name);
        node->setTitle(name);
        node->setVersion(ei.firstChildElement("version").text());
        node->setHardware(ei.firstChildElement("hardware").text());
        node->setInfoValid(true);
        read(e,node,fmt);
        icnt++;
      }
      QCoreApplication::processEvents();
    }
    return icnt;
  }
  //load already present nodes - values
  for(QDomElement e=dom.firstChildElement("node");!e.isNull();e=e.nextSiblingElement(e.tagName())){
    QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
    int fmt=e.attribute("fmt").toInt();
    if(nodes->snMap.contains(sn)){
      NodeItem *node=nodes->snMap.value(sn);
      read(e,node,fmt);
      icnt++;
    }
  }
  return icnt;
}
//=============================================================================
int NodesXml::read(QDomNode dom, NodeItem *node, int fmt) const
{
  //dom is "node" tag
  int lcnt=0;
  bool bDict=(!node->dictValid()) && node->allFields.isEmpty() && node->commands.cmd.isEmpty();
  if(bDict){ //load node dictionary
    //fields inf struct
    QDomElement e=dom.firstChildElement("fields");
    if(e.isNull())return 0; //no struct exists in dom
    //create all fields empty
    int cnt=e.attribute("cnt").toInt();
    for(quint16 id=0;id<cnt;id++){
      node->allFields.append(new NodeField(node,id));
    }
    //read commands
    for(QDomElement e=dom.firstChildElement("commands").firstChildElement("command");!e.isNull();e=e.nextSiblingElement(e.tagName())){
      uint cmd=e.attribute("id").toUInt();
      QString sname=e.firstChildElement("name").text();
      QString sdescr=e.firstChildElement("descr").text();
      if(sname.size() && sdescr.size()){
        node->addCommand(cmd,sname,sdescr);
      }
    }
    node->commands.valid=true;
    //fields struct
    for(QDomElement e=dom.firstChildElement("fields").firstChildElement("field");!e.isNull();e=e.nextSiblingElement(e.tagName())){
      NodeField *f=node->allFields.value(e.attribute(fmt==0?"idx":"id").toInt(),NULL);
      if(!f)continue;
      QString r_name=e.toElement().attribute("name");
      if(r_name.isEmpty()) continue;
      if(fmt==0){
        //old format - expand descr, name. etc
        QDomElement eS=e.firstChildElement("struct");
        if(eS.isNull())continue;
        QString stype=eS.firstChildElement("type").text();
        for(int i=0;i<ft_cnt;i++){
          QString s=f->ftypeString(i);
          if(!s.size())break;
          if(s!=stype)continue;
          f->ftype=i;
          break;
        }
        QString r_descr=eS.firstChildElement("descr").text();
        QStringList r_opts=eS.firstChildElement("opts").text().split(',',QString::SkipEmptyParts);
        if(r_opts.size()<=1)r_opts.clear();
        //preprocess expand
        if(r_name.contains("[")){
          f->setArray(r_name.section("[",1,1).section("]",0,0).toInt());
          r_name=r_name.left(r_name.indexOf('['));
        }
        f->setName(r_name);
        //trim title
        if(r_name.contains("_")){
          r_name.remove(0,r_name.indexOf('_')+1);
        }
        //units from descr
        f->setUnits("");
        if(r_descr.contains('[')){
          QString s=r_descr.mid(r_descr.lastIndexOf('[')+1);
          r_descr=r_descr.left(r_descr.indexOf('['));
          s=s.left(s.indexOf(']'));
          if(!s.contains("..")) f->setUnits(s);
        }
        //groups from descr
        f->groups.clear();
        while(r_descr.contains(':')){
          QString s=r_descr.left(r_descr.indexOf(':')).trimmed();
          f->groups.append(s);
          r_descr=r_descr.remove(0,r_descr.indexOf(':')+1).trimmed();
          if(r_name.contains('_') && r_name.left(r_name.indexOf('_')).toLower()==s.toLower())
            r_name.remove(0,r_name.indexOf('_')+1);
        }
        f->setTitle(r_name);
        f->setDescr(r_descr);
        f->setEnumStrings(r_opts);
      }else{
        //current format
        f->setName(r_name);
        f->setTitle(e.firstChildElement("title").text());
        f->setDescr(e.firstChildElement("descr").text());
        f->setUnits(e.firstChildElement("units").text());
        f->setDefaultValue(e.firstChildElement("defValue").text());
        f->setArray(e.firstChildElement("array").text().toInt());
        f->setEnumStrings(e.firstChildElement("opts").text().split(',',QString::SkipEmptyParts));
        f->groups=e.firstChildElement("sect").text().split('/',QString::SkipEmptyParts);
        QString stype=e.firstChildElement("ftype").text();
        for(int i=0;i<ft_cnt;i++){
          QString s=f->ftypeString(i);
          if(!s.size())break;
          if(s!=stype)continue;
          f->ftype=i;
          break;
        }
        if(f->ftype<0)continue;
      }
      if(f->ftype>=0 && f->ftype<ft_cnt && (!r_name.isEmpty())){
        f->setDictValid(true);
      }
    }
  }
  if(!node->dictValid()){
    qWarning("Incomplete node dictionary in xml");
    return 0;
  }
  //read field values
  for(QDomElement e=dom.firstChildElement("fields").firstChildElement("field");!e.isNull();e=e.nextSiblingElement(e.tagName())){
    NodeField *f=NULL;
    QString fname=e.attribute("name");
    int array=e.firstChildElement("array").text().toInt();
    if(fmt==0 && fname.contains('[')){
      array=fname.section("[",1,1).section("]",0,0).toInt();
      fname=fname.left(fname.indexOf('['));
    }

    if(bDict){
      f=node->allFields.value(e.attribute(fmt==0?"idx":"id").toInt(),NULL);
      if(!f)return 0;
    }

    //find field by name
    foreach(NodeField *i,node->allFields){
      if(!i->dictValid())continue;
      if(i->array()!=array)continue;
      if(i->name()!=fname)continue;
      f=i;
      break;
    }
    //try find excl array size []
    if((!f) && array>0){
      foreach(NodeField *i,node->allFields){
        if(!i->dictValid())continue;
        if(i->array()<=0)continue;
        if(i->name()!=fname)continue;
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
        foreach(NodeField *i,node->allFields){
          if(!i->dictValid())continue;
          if(i->array()!=array)continue;
          if(i->name()!=vname)continue;
          f=i;
          break;
        }
      }
    }
    //try load field values
    if(!f){
      qWarning("%s: %s/%s",tr("Field missing").toUtf8().data(),node->title().toUtf8().data(),fname.toUtf8().data());
      continue;
    }
    if(!(fname==f->name() && array==f->array())){
      qWarning("%s: %s/%s -> %s",tr("Field map").toUtf8().data(),node->title().toUtf8().data(),fname.toUtf8().data(),f->name().toUtf8().data());
    }
    read(e,f);
    lcnt++;
  }
  return lcnt;
}
//=============================================================================
bool NodesXml::read(QDomNode dom, NodeField *field) const
{
  //dom is "field" tag
  //read field value
  int lcnt=0;
  for(QDomElement e=dom.firstChildElement("value");!e.isNull();e=e.nextSiblingElement(e.tagName())){
    NodeField *f=field;
    if(field->size()>0){
      f=static_cast<NodeField*>(field->child(e.attribute("idx").toInt()));
    }else if(field->name()=="comment" && e.text().isEmpty()){
      f=NULL;
    }
    if(!f)continue;
    f->setValue(e.text());
    lcnt++;
    f->setDataValid(true);
  }
  field->setDataValid(true);
  return lcnt>0;
}
//=============================================================================
//=============================================================================
int NodesXml::import(QDomNode dom) const
{
  int fmt=dom.toElement().attribute("fmt").toInt();
  int icnt=0;
  QHash<NodeItem*,QByteArray> snmap;
  QHash<NodeItem*,int>nmap;
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
    NodeItem *node=NULL;
    const QByteArray &sn=QByteArray::fromHex(ssn.toUtf8());
    //match by serial numner
    priority=0;
    if(nodes->snMap.contains(sn)){
      node=nodes->snMap.value(sn);
      nmap.insert(node,priority);
      //qDebug()<<"Node by sn: "<<node->name<<node_name<<comment;
    }
    //find matching node by name and comment
    priority++;
    if(!node){
      if(comment.size()){
        foreach(NodeItem *i,nodes->snMap.values())
          if(i->title()==node_name && i->status()==comment){
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
        foreach(NodeItem *i,nodes->snMap.values())
          if(i->title()==node_name && i->status().isEmpty()){
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
        foreach(NodeItem *i,nodes->snMap.values())
          if(i->title()==node_name){
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
        foreach(NodeItem *i,nodes->snMap.values())
          if(i->title().endsWith(node_name)){
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
    NodeItem *node=snmap.key(sn,NULL);
    if(node)read(ne,node,fmt);
  }
  return icnt;
}
//=============================================================================
