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
//=============================================================================
FactSystemJS::FactSystemJS(QObject *parent)
{
  js=new QJSEngine(parent);
  js->installExtensions(QJSEngine::TranslationExtension | QJSEngine::ConsoleExtension);
}
//=============================================================================
void FactSystemJS::jsSync(QQmlEngine *e,QObject *obj)
{
  QQmlEngine::setObjectOwnership(obj,QQmlEngine::CppOwnership);
  e->globalObject().setProperty(obj->objectName(),e->newQObject(obj));
}
//=============================================================================
QJSValue FactSystemJS::jsSync(QQmlEngine *e, Fact *factItem, QJSValue parent) //recursive
{
  QQmlEngine::setObjectOwnership(factItem,QQmlEngine::CppOwnership);
  QJSValue js_factItem=e->newQObject(factItem);
  parent.setProperty(factItem->name(),js_factItem);
  foreach(FactTree *i,factItem->childItemsTree())
    jsSync(e,static_cast<Fact*>(i),js_factItem);
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
void FactSystemJS::jsRegisterFunctions()
{
  //system functions and objects
  jsRegister("help()",QObject::tr("print commands help"),"m.help();");
  jsRegister("req(n)",QObject::tr("request var n from UAV"),"mandala[n].request();");
  jsRegister("send(n)",QObject::tr("send var n to UAV"),"mandala[n].send();");
  jsRegister("nodes()",QObject::tr("rescan bus nodes"),"print('nodes:');mandala.req_nodes();");
  jsRegister("nstat()",QObject::tr("print nodes status"),"print('nodes statistics:');mandala.req_nstat();");
  jsRegister("serial(p,v)",QObject::tr("send data v to serial port ID p"),"mandala.scr_serial(p,v);");
  jsRegister("can(i,e,v)",QObject::tr("send data v to CAN ID i with IDE e"),"mandala.scr_can(i,e,v);");
  jsRegister("vmexec(f)",QObject::tr("execute function on VMs"),"mandala.scr_vmexec(f);");
  jsRegister("sleep(n)",QObject::tr("sleep n milliseconds"),"mandala.scr_sleep(n);");
  jsRegister("next()",QObject::tr("switch to next vehicle"),"mandala.global.changeCurrentNext();");
  jsRegister("prev()",QObject::tr("switch to previous vehicle"),"mandala.global.changeCurrentPrev();");

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
