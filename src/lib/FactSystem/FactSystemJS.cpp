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
#include "FactSystemJS.h"
#include "AppDirs.h"
#include <Vehicles.h>
//=============================================================================
FactSystemJS::FactSystemJS(QObject *parent)
 : FactSystemApp(parent)
{
  js=new QQmlEngine(this);
  js->installExtensions(QJSEngine::TranslationExtension | QJSEngine::ConsoleExtension);
  jsRegisterFunctions();

  js->rootContext()->setContextProperty("font_narrow","Bebas Neue");
  js->rootContext()->setContextProperty("font_mono","FreeMono");
  js->rootContext()->setContextProperty("font_condenced","Ubuntu Condensed");


  // script include file (default)
  QFile jsFile(AppDirs::res().filePath("scripts/gcs.js"));
  if (jsFile.open(QIODevice::ReadOnly)){
    QTextStream stream(&jsFile);
    QString contents = stream.readAll();
    jsFile.close();
    jsexec(contents);
  }
  // script include file (user commands)
  QFile jsFile2(AppDirs::scripts().filePath("gcs.js"));
  if (jsFile2.open(QIODevice::ReadOnly)){
    QTextStream stream(&jsFile2);
    QString contents = stream.readAll();
    jsFile2.close();
    jsexec(contents);
  }


  /*static QVariantMap tmap;
  tmap["one"]="two";
  js->globalObject().setProperty("tmap",js->toScriptValue(tmap));*/


  //add is queued to wait inherited constructors
  //connect(this,&Fact::itemAdded,this,&FactSystemJS::jsAddItem);//,Qt::QueuedConnection);
  //connect(this,&Fact::itemRemoved,this,&FactSystemJS::jsRemoveItem);//,Qt::QueuedConnection);
}
//=============================================================================
void FactSystemJS::jsSyncObject(QObject *obj)
{
  QQmlEngine::setObjectOwnership(obj,QQmlEngine::CppOwnership);
  js->globalObject().setProperty(obj->objectName(),js->newQObject(obj));
}
//=============================================================================
void FactSystemJS::jsSync(Fact *item)
{
  QList<FactTree*> list=item->pathList();
  QJSEngine *e=js;
  QJSValue v=e->globalObject();
  for(int i=list.size()-1;i>0;--i){
    Fact *fact=static_cast<Fact*>(list.at(i));
    QJSValue vp=v.property(fact->name());
    if(vp.isUndefined() || (!vp.isQObject()) || vp.toQObject()!=fact){
      vp=e->newQObject(fact);
      v.setProperty(fact->name(),vp);
    }
    v=vp;
  }
  jsSync(item,v);
  js->collectGarbage();
}
//=============================================================================
QJSValue FactSystemJS::jsSync(Fact *factItem, QJSValue parent) //recursive
{
  //qDebug()<<factItem->path();
  QQmlEngine::setObjectOwnership(factItem,QQmlEngine::CppOwnership);
  QJSValue js_factItem=js->newQObject(factItem);//js->toScriptValue<Fact*>(factItem);//
  parent.setProperty(factItem->name(),js_factItem);
  foreach(FactTree *i,factItem->childItems())
    jsSync(static_cast<Fact*>(i),js_factItem);
  if(!factItem->actions.isEmpty()){
    QJSValue js_actions=js->newObject();
    foreach(FactAction *i,factItem->actions)
      js_actions.setProperty(i->name(),js->newQObject(i));
    js_factItem.setProperty("action",js_actions);
  }
  return js_factItem;
}
//=============================================================================
QJSValue FactSystemJS::jsexec(const QString &s)
{
  QJSValue result;
  result=js->evaluate(s);
  if(result.isError()){
    qWarning("%s",result.toString().toUtf8().data());
  }
  return result;
}
//=============================================================================
void FactSystemJS::jsRegister(QString fname, QString description, QString body)
{
  jsexec(QString("function %1 { %2;};").arg(fname).arg(body));
  js_descr[fname]=description;
}
//=============================================================================
//=============================================================================
void FactSystemJS::jsAddItem(FactTree *item)
{
  //qDebug()<<static_cast<Fact*>(item)->path();
  QQmlEngine::setObjectOwnership(item,QQmlEngine::CppOwnership);
  QJSEngine *e=js;
  //find the parents tree, last item in list = this
  QList<FactTree*> list=item->pathList();
  QJSValue v=e->globalObject();
  for(int i=list.size()-1;i>=0;--i){
    Fact *fact=static_cast<Fact*>(list.at(i));
    QJSValue vp=v.property(fact->name());
    if(vp.isUndefined() || (!vp.isQObject()) || vp.toQObject()!=fact){
      vp=e->newQObject(fact);
      v.setProperty(fact->name(),vp);
      qDebug()<<fact->path();
    }
    v=vp;
  }
  js->collectGarbage();
}
void FactSystemJS::jsRemoveItem(FactTree *item)
{
  //qDebug()<<"jsRemoveItem"<<static_cast<Fact*>(item)->path();
  QJSEngine *e=js;
  //find the parents tree, last item in list = this
  QList<FactTree*> list=item->pathList();
  QJSValue v=e->globalObject();
  for(int i=list.size()-1;i>0;--i){
    Fact *fact=static_cast<Fact*>(list.at(i));
    QJSValue vp=v.property(fact->name());
    if(vp.isUndefined())return; //no parents?
    v=vp;
  }
  v.deleteProperty(item->name());
}
//=============================================================================
void FactSystemJS::alias(FactTree *item,QString aliasName)
{
  //jsexec(QString("var %1=%2").arg(aliasName).arg(item->path()));
  //QML only
  js->rootContext()->setContextProperty(aliasName,item);
}
//=============================================================================
//=============================================================================
void FactSystemJS::jsRegisterFunctions()
{
  //system functions and objects
  jsRegister("help()",QObject::tr("print commands help"),"app.help();");
  jsRegister("req(n)",QObject::tr("request var n from UAV"),"app.vehicles.current.mandala[n].request();");
  jsRegister("send(n)",QObject::tr("send var n to UAV"),"m[n].send();");
  jsRegister("nodes()",QObject::tr("rescan bus nodes"),"print('nodes:');m.req_nodes();");
  jsRegister("nstat()",QObject::tr("print nodes status"),"print('nodes statistics:');app.vehicles.current.nodes.nstat();");
  jsRegister("serial(p,v)",QObject::tr("send data v to serial port ID p"),"app.vehicles.current.sendSerial(p,v);");
  jsRegister("vmexec(f)",QObject::tr("execute function on VMs"),"app.vehicles.current.vmexec(f);");
  jsRegister("sleep(n)",QObject::tr("sleep n milliseconds"),"app.sleep(n);");
  jsRegister("next()",QObject::tr("switch to next vehicle"),"app.vehicles.selectNext();");
  jsRegister("prev()",QObject::tr("switch to previous vehicle"),"app.vehicles.selectPrev();");

  //some helper functions
  jsRegister("trigger(v,a,b)",QObject::tr("trigger value of v to a or b"),"if(v==a)return b; else return a;");
  jsRegister("bound(v)",QObject::tr("wrap angle -180..+180"),"while(v>=180)v-=360;while(v<-180)v+=360;return v;");
  jsRegister("ls(a,b)",QObject::tr("print members of type b for scope a"),"for(var i in a)if(typeof(a[i])==b || !b)print(i+\" - \"+typeof(a[i]));");
  jsRegister("vars(a)",QObject::tr("print variables for scope a"),"if(arguments.length==0)a=this;for(var i in a)if(typeof(a[i])=='number')print(i+\"=\"+a[i]);");
  jsRegister("func(a)",QObject::tr("print functions for scope a"),"if(arguments.length==0)a=this;for(var i in a)if(typeof(a[i])=='function')print(i);");
  //predefined commands
  jsRegister("ahrs()",QObject::tr("reset AHRS"),"req('roll');");
  jsRegister("zrc()",QObject::tr("reset pilot controls"),"rc_roll=0;rc_pitch=0;rc_throttle=0;rc_yaw=0;");
  jsRegister("zps()",QObject::tr("reset barometric altitude on ground"),"altps_gnd=0;");
  jsRegister("ned()",QObject::tr("reset local GPS coordinates"),"home_lat=gps_lat;home_lon=gps_lon;home_hmsl=gps_hmsl;");
  jsRegister("hmsl()",QObject::tr("reset local GPS altitude"),"home_hmsl=gps_hmsl;");
}
//=============================================================================
void FactSystemJS::help()
{
  QString s;
  s+="<html><table width=100%>";
  foreach(const QString &cmd,js_descr.keys()){
    s+="<tr><td valign='middle' style='background-color: #111; font-family: Monospace;'>";
    s+="<NOBR>&nbsp;<font color=cyan>";
    s+=cmd;
    s+="</font></NOBR></td><td width='100%'>";
    s+="<font color=gray> &nbsp;"+js_descr.value(cmd)+"</font>";
    s+="</td></tr>";
  }
  s+="</table></font>";
  qDebug("%s",s.toUtf8().data());
}
//=============================================================================
//=============================================================================
QByteArray FactSystemJS::jsToArray(QJSValue data)
{
  //qDebug()<<portID<<data.toString()<<data.isArray()<<data.toVariant().toByteArray().toHex();
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
    ba.append((unsigned char)data.toInt());
  }else return QByteArray();
  return ba;
}
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================



