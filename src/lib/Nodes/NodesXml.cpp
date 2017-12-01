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
  : QObject(parent), nodes(parent)
{
}
//=============================================================================
void NodesXml::write(QDomNode dom, NodeItem *node) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("node"));
  dom.toElement().setAttribute("sn",QString(node->sn.toHex().toUpper()));
  dom.toElement().setAttribute("name",node->title());

  QDomNode e=dom.appendChild(doc.createElement("info"));
  e.appendChild(doc.createElement("version")).appendChild(doc.createTextNode(node->version()));
  e.appendChild(doc.createElement("hardware")).appendChild(doc.createTextNode(node->hardware()));

  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(node->hash().toHex().toUpper()));
  dom.appendChild(doc.createElement("timestamp")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));

  if(node->commands.cmd.size()){
    for(int i=0;i<node->commands.cmd.size();i++){
      QDomNode e=dom.appendChild(doc.createElement("command"));
      e.toElement().setAttribute("cmd",QString::number(node->commands.cmd.at(i)));
      e.appendChild(doc.createElement("name")).appendChild(doc.createTextNode(node->commands.name.at(i)));
      e.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(node->commands.descr.at(i)));
    }
  }

  e=dom.appendChild(doc.createElement("fields"));
  e.toElement().setAttribute("cnt",QString::number(node->allFields.size()));
  e.toElement().setAttribute("hash",node->conf_hash);

  QDomNode domf=e;
  foreach(NodeField *f,node->allFields){
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
        e.appendChild(doc.createTextNode(subf->valueToText()));
      }
    }else{
      dom.appendChild(doc.createElement("value")).appendChild(doc.createTextNode(f->valueToText()));
    }
  }
}
//=============================================================================
void NodesXml::write(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement("nodes"));
  dom.toElement().setAttribute("href","http://www.uavos.com/");
  dom.toElement().setAttribute("vehicle",nodes->vehicle->title());
  dom.toElement().setAttribute("version",FactSystem::version());
  dom.appendChild(doc.createElement("hash")).appendChild(doc.createTextNode(QString(nodes->hash().toHex().toUpper())));
  dom.appendChild(doc.createElement("timestamp")).appendChild(doc.createTextNode(QDateTime::currentDateTimeUtc().toString()));
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
    //int pcnt=dom.childNodes().size();
    //load and construct nodes from file
    dom.childNodes().size();
    QDomElement e=dom.firstChildElement("node");
    while(!e.isNull()){
      QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
      QString name=e.attribute("name");
      QDomElement ei=e.firstChildElement("info");
      if(!ei.isNull()){
        NodeItem *node=nodes->nodeCheck(sn);
        node->setName(name);
        node->setTitle(name);
        node->setVersion(ei.firstChildElement("version").text());
        node->setHardware(ei.firstChildElement("hardware").text());
        node->setInfoValid(true);
        read(e,node);
        icnt++;
      }
      e=e.nextSiblingElement(e.tagName());
      QCoreApplication::processEvents();
    }
    return icnt;
  }
  //load already present nodes - values
  QDomElement e=dom.firstChildElement("node");
  while(!e.isNull()){
    QByteArray sn=QByteArray::fromHex(e.attribute("sn").toUtf8());
    if(nodes->snMap.contains(sn)){
      NodeItem *node=nodes->snMap.value(sn);
      read(e,node);
      icnt++;
    }
    e=e.nextSiblingElement(e.tagName());
  }
  return icnt;
}
//=============================================================================
int NodesXml::read(QDomNode dom, NodeItem *node) const
{
  //dom is "node" tag
  int lcnt=0;
  bool bDict=!node->valid();
  if(bDict){ //load node dictionary
    node->commands.cmd.clear();
    node->commands.name.clear();
    node->commands.descr.clear();
    node->commands.valid=false;
    //fields inf struct
    QDomElement e=dom.firstChildElement("fields");
    if(e.isNull())return 0; //no struct exists in dom
    //check mn in dom and this node
    int cnt=e.attribute("cnt").toInt();
    //uint size=e.attribute("size").toInt();
    QString hash=e.attribute("hash");
    if(node->allFields.isEmpty()){
      node->conf_hash=hash;
      for(quint16 id=0;id<cnt;id++){
        node->allFields.append(new NodeField(node,id));
      }
    }else if(cnt!=node->allFields.size() || hash!=node->conf_hash){
      //conf_inf in file and node not the same
      return 0;
    }
    //commands
    e=dom.firstChildElement("command");
    if(e.isNull()) node->commands.valid=true; //node without commands
    while(!e.isNull()){
      uint cmd=e.attribute("cmd").toUInt();
      QString sname=e.firstChildElement("name").text();
      QString sdescr=e.firstChildElement("descr").text();
      if(sname.size() && sdescr.size()){
        node->commands.cmd.append(cmd);
        node->commands.name.append(sname);
        node->commands.descr.append(sdescr);
        node->commands.valid=true;
        //qDebug()<<cmd<<sname<<sdescr;
      }
      e=e.nextSiblingElement(e.tagName());
    }
    //fields struct
    for(QDomElement e=dom.firstChildElement("fields").firstChildElement("field");!e.isNull();e=e.nextSiblingElement(e.tagName())){
      NodeField *field=node->allFields.value(e.attribute("idx").toInt(),NULL);
      if(!field)continue;
      //read(e,f);
      QString r_name=e.toElement().attribute("name");
      field->setName(r_name);
      field->setTitle(r_name);
      field->conf_name=r_name;
      //load field struct
      QDomElement eS=e.firstChildElement("struct");
      if(eS.isNull())continue;
      QString stype=eS.firstChildElement("type").text();
      for(int i=0;i<ft_cnt;i++){
        QString s=field->ftypeString(i);
        if(!s.size())break;
        if(s!=stype)continue;
        field->ftype=i;
        break;
      }
      field->conf_descr=eS.firstChildElement("descr").text();
      field->setDescr(field->conf_descr);
      QStringList st=eS.firstChildElement("opts").text().split(',',QString::SkipEmptyParts);
      if(st.size()>1)field->setEnumStrings(st);
      if(field->ftype<ft_cnt && (!r_name.isEmpty())){
        field->setValid(true);
      }
      //node->nodes->setProgress(node->allFields.size()*100/(field->id+1));
      //QCoreApplication::processEvents();
      //qDebug()<<"loaded"<<field->path()<<field->valid();
    }
  }
  if(!node->valid()){
    qWarning("Incomplete node dictionary in xml");
    return 0;
  }
  //read field values
  for(QDomElement e=dom.firstChildElement("fields").firstChildElement("field");!e.isNull();e=e.nextSiblingElement(e.tagName())){
    NodeField *f=NULL;
    QString fname=e.attribute("name");

    if(bDict){
      f=node->allFields.value(e.attribute("idx").toInt(),NULL);
      if(!f)return 0;
    }

    //find field by conf_name
    foreach(NodeField *i,node->allFields){
      if((!i->valid())||i->conf_name!=fname)continue;
      f=i;
      break;
    }
    //try find excl array size []
    if((!f) && fname.contains('[')){
      foreach(NodeField *i,node->allFields){
        if(!i->valid())continue;
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
        foreach(NodeField *i,node->allFields){
          if((!i->valid())||i->conf_name!=vname)continue;
          f=i;
          break;
        }
      }
    }
    //anyway, try old formats
    if(!f) qWarning("%s: %s/%s",tr("Field missing").toUtf8().data(),node->title().toUtf8().data(),fname.toUtf8().data());
    if(f){
      lcnt++;
      if(fname!=f->conf_name){
        qWarning("%s: %s/%s -> %s",tr("Field map").toUtf8().data(),node->title().toUtf8().data(),fname.toUtf8().data(),f->conf_name.toUtf8().data());
      }
      read(e,f);
    }
  }
  return lcnt;
}
//=============================================================================
bool NodesXml::read(QDomNode dom, NodeField *field) const
{
  //dom is "field" tag, with idx==fnum
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
    if(node)read(ne,node);
  }
  return icnt;
}
//=============================================================================
