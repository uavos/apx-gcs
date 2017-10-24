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
#include <QtQml>
#include <QQmlEngine>

#include "FactSystem.h"
#define VSTR_IMPL(a) #a
#define VSTR(a) VSTR_IMPL(a)
//=============================================================================
FactSystem::FactSystem(QObject *parent)
 : QObject(parent)
{
  _instance=this;

  setObjectName("app");

  _tree=new Fact(NULL,objectName(),tr("System tree"),QCoreApplication::applicationName(),Fact::RootItem,Fact::NoData);

  _tree->setFlatModel(true);

  // app constants and global facts
  Fact *item;

  item=new Fact(_tree,"version",tr("Version"),tr("Application version"),Fact::FactItem,Fact::ConstData);
  QString s=VSTR(VERSION);
  if(s.isEmpty())s=tr("unknown");
  item->setValue(s);
  item->setEnabled(false);

  item=new Fact(_tree,"branch",tr("Branch"),tr("Source code repository branch"),Fact::FactItem,Fact::ConstData);
  item->setValue(VSTR(BRANCH));
  item->setEnabled(false);

  item=new Fact(_tree,"dev",tr("Development mode"),"",Fact::FactItem,Fact::NoData);
  item->setEnabled(false);
#ifdef __ANDROID__
  item->setValue(false);
#else
  item->setValue(QCoreApplication::applicationDirPath().startsWith(QDir::homePath()));
#endif
  item->setVisible(item->value().toBool());

  // QML types register
  qmlRegisterUncreatableType<Fact>("GCS.FactSystem", 1, 0, "Fact", "Reference only");
}
FactSystem::~FactSystem()
{
  _tree->deleteLater();
}
FactSystem * FactSystem::_instance=NULL;
//=============================================================================
const QString FactSystem::ApplicationSection=tr("Application");
const QString FactSystem::MandalaSection=tr("Application");
//=============================================================================
//=============================================================================
Fact * FactSystem::tree()
{
  return _instance->_tree;
}
//=============================================================================
//=============================================================================
void FactSystem::appendTree(Fact *tree)
{
  FactSystem::tree()->addItem(tree);
}
//=============================================================================
QJSValue FactSystem::_syncJS(QQmlEngine *e, Fact *factItem, QJSValue parent) //recursive
{
  QQmlEngine::setObjectOwnership(factItem,QQmlEngine::CppOwnership);
  QJSValue js_factItem=e->newQObject(factItem);
  parent.setProperty(factItem->name(),js_factItem);
  foreach(FactTree *i,factItem->childItemsTree())
    _syncJS(e,static_cast<Fact*>(i),js_factItem);
  return js_factItem;
}
//=============================================================================
void FactSystem::syncJS(QQmlEngine *e)
{
  QQmlEngine::setObjectOwnership(_instance,QQmlEngine::CppOwnership);
  e->globalObject().setProperty("sys",e->newQObject(_instance));

  _instance->_syncJS(e,FactSystem::tree(),e->globalObject());
}
//=============================================================================
//=============================================================================
QVariant FactSystem::value(const QString &namePath)
{
  Fact *f=tree()->fact(namePath);
  if(!f){
    qWarning("FactSystem fact not found: %s",namePath.toUtf8().data());
    return QVariant();
  }
  if(f->dataType()==Fact::EnumData)return f->text();
  return f->value();
}
//=============================================================================
//=============================================================================
//=============================================================================
QString FactSystem::latToString(double v)
{
  double lat=fabs(v);
  double lat_m=60*(lat-floor(lat)),lat_s=60*(lat_m-floor(lat_m)),lat_ss=100*(lat_s-floor(lat_s));
  return QString().sprintf("%c %g%c%02g'%02g.%02g\"",(v>=0)?'N':'S',floor(lat),176,floor(lat_m),floor(lat_s),floor(lat_ss));
}
QString FactSystem::lonToString(double v)
{
  double lat=fabs(v);
  double lat_m=60*(lat-floor(lat)),lat_s=60*(lat_m-floor(lat_m)),lat_ss=100*(lat_s-floor(lat_s));
  return QString().sprintf("%c %g%c%02g'%02g.%02g\"",(v>=0)?'E':'W',floor(lat),176,floor(lat_m),floor(lat_s),floor(lat_ss));
}
double FactSystem::latFromString(QString s)
{
  bool ok;
  int i;
  s=s.simplified();
  if(QString("NS").contains(s.at(0))){
    bool bN=s.at(0)=='N';
    s=s.remove(0,1).trimmed();
    i=s.indexOf(QChar(176));
    double deg=s.left(i).toDouble(&ok);
    if(!ok)return 0;
    s=s.remove(0,i+1).trimmed();
    i=s.indexOf('\'');
    double min=s.left(i).toDouble(&ok);
    if(!ok)return 0;
    s=s.remove(0,i+1).trimmed();
    i=s.indexOf('\"');
    double sec=s.left(i).toDouble(&ok);
    if(!ok)return 0;
    deg=deg+min/60.0+sec/3600.0;
    return bN?deg:-deg;
  }
  return s.toDouble();
}
double FactSystem::lonFromString(QString s)
{
  s=s.simplified();
  if(QString("EW").contains(s.at(0)))
    s[0]=(s.at(0)=='E')?'N':'S';
  return latFromString(s);
}
QString FactSystem::distanceToString(uint v)
{
  if(v>=1000000)return QString("%1km").arg(v/1000.0,0,'f',0);
  if(v>=1000)return QString("%1km").arg(v/1000.0,0,'f',1);
  return QString("%1m").arg((uint)v);
}
QString FactSystem::timeToString(uint v)
{
  if(v==0)return "--:--";
  qint64 d=(qint64)v/(24*60*60);
  if(d<=0)return QString("%1").arg(QTime(0,0,0).addSecs(v).toString("hh:mm"));
  return QString("%1d%2").arg(d).arg(QTime(0,0,0).addSecs(v).toString("hh:mm"));
}
uint FactSystem::timeFromString(QString s)
{
  uint t=0;
  s=s.trimmed().toLower();
  if(s.contains('d')){
    QString ds=s.left(s.indexOf('d')).trimmed();
    s=s.remove(0,s.indexOf('d')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(24*60*60));
  }
  if(s.contains('h')){
    QString ds=s.left(s.indexOf('h')).trimmed();
    s=s.remove(0,s.indexOf('h')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(60*60));
  }
  if(s.contains('m')){
    QString ds=s.left(s.indexOf('m')).trimmed();
    s=s.remove(0,s.indexOf('m')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(60));
  }
  if(s.contains('s')){
    QString ds=s.left(s.indexOf('s')).trimmed();
    s=s.remove(0,s.indexOf('s')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv);
    s.clear();
  }
  if(s.contains(':')){
    QString ds=s.left(s.indexOf(':')).trimmed();
    s=s.remove(0,s.indexOf(':')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(60*60));
    if(s.contains(':')){
      QString ds=s.left(s.indexOf(':')).trimmed();
      s=s.remove(0,s.indexOf(':')+1).trimmed();
      bool ok=false;
      double dv=ds.toDouble(&ok);
      if(ok && dv>0)t+=floor(dv*(double)(60));
    }else{
      bool ok=false;
      double dv=s.toDouble(&ok);
      if(ok && dv>0)t+=floor(dv*(double)(60));
      s.clear();
    }
  }
  if(!s.isEmpty()){
    bool ok=false;
    double dv=s.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv);
  }
  return t;
}
//=============================================================================
void FactSystem::toolTip(QString tooltip)
{
  qDebug()<<":: "<<tooltip;
}
double FactSystem::limit(double v,double min,double max)
{
  if(v<min)return min;
  if(v>max)return max;
  return v;
}
double FactSystem::angle(double v)
{
  const double span=180.0;
  const double dspan=span*2.0;
  return v-floor(v/dspan+0.5)*dspan;
}
double FactSystem::angle360(double v)
{
  while(v<0) v+=360.0;
  while(v>=360.0) v-=360.0;
  return v;
}
double FactSystem::angle90(double v)
{
  const double span=90.0;
  const double dspan=span*2.0;
  return v-floor(v/dspan+0.5)*dspan;
}
//=============================================================================
