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
#include "AppSettings.h"
#include "AppDirs.h"
//=============================================================================
AppSettings::AppSettings(FactSystem *parent)
 : Fact(parent->tree(),"settings",tr("Application settings"),tr("Global application preferences"),RootItem,NoData) //root
{
  _instance=this;

  setSection(FactSystem::ApplicationSection);

  QDir spath(AppDirs::user().absoluteFilePath("Preferences"));
  if(!spath.exists())spath.mkpath(".");
  m_settings=new QSettings(spath.absoluteFilePath(QCoreApplication::applicationName()+".ini"),QSettings::IniFormat,this);

  Fact *item, *item2;
  QString sect, sect2;

  //sect=tr("Interface");
  item=new AppSettingFact(m_settings,this,"sounds",tr("Sounds"),tr("Enable all application sounds and voice"),sect,BoolData,true);

  item=new AppSettingFact(m_settings,this,"lang",tr("Language"),tr("Interface localization"),sect,EnumData,0);
  item2=new Fact(item,"default","","",ConstItem,ItemIndexData);
  QDir langp(AppDirs::lang());
  foreach(QFileInfo f,langp.entryInfoList(QStringList()<<"*.qm"))
    item2=new Fact(item,f.baseName(),"","",ConstItem,ItemIndexData);

  item=new AppSettingFact(m_settings,this,"voice",tr("Voice"),tr("Speaking voice"),sect,EnumData,0);
  item2=new Fact(item,"default","","",ConstItem,ItemIndexData);
  QDir voicep(AppDirs::res().filePath("audio/speech"));
  foreach(QString s,voicep.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
    item2=new Fact(item,s,"","",ConstItem,ItemIndexData);

  item=new AppSettingFact(m_settings,this,"opengl",tr("Accelerate graphics"),tr("Enable OpenGL graphics when supported"),sect,BoolData,false);
  item=new AppSettingFact(m_settings,this,"smooth",tr("Smooth animations"),tr("Enable animations and antialiasing"),sect,BoolData,true);
  item=new AppSettingFact(m_settings,this,"showdescr",tr("Show descriptions"),tr("Enable menu items description text"),sect,BoolData,true);


/*
  sect=tr("Datalink");
  item=new AppSettingFact(m_settings,this,"readonly",tr("Read only"),tr("Block all uplink data"),sect,BoolData,false);
  connect(item,&AppSettingFact::valueChanged,this,&AppSettings::readonlyChanged);

  item=new AppSettingsPorts(this,sect);

  item=new Fact(this,"network",tr("Network"),tr("Networking preferences"),GroupItem,NoData);
  item->setSection(sect);
  sect2=tr("Server");
  item2=new AppSettingFact(m_settings,item,"name",tr("Name"),tr("Local server customized name"),sect2,TextData);
  item2=new AppSettingFact(m_settings,item,"pass",tr("Password"),tr("Local server access password"),sect2,TextData);
  item2=new AppSettingFact(m_settings,item,"extctr",tr("Allow external controls"),tr("Don't block uplink from clients"),sect2,BoolData,true);
  sect2=tr("Other");
  item2=new AppSettingFact(m_settings,item,"proxy",tr("HTTP proxy"),tr("Proxy for web data requests"),sect2,TextData);
*/


  //load all settings
  AppSettingFact::loadSettings(this);
}
AppSettings * AppSettings::_instance=NULL;
//=============================================================================
//=============================================================================
AppSettingFact::AppSettingFact(QSettings *settings, Fact *parent, QString name, QString label, QString descr, QString section, DataType dataType, QVariant defaultValue)
 : Fact(parent,name,label,descr,Fact::FactItem,dataType),
   m_settings(settings),
   m_defaultValue(defaultValue)
{
  setSection(section);
  list.append(this);
}
QList<AppSettingFact*> AppSettingFact::list;
bool AppSettingFact::setValue(const QVariant &v)
{
  QVariant vx=v;
  if(vx.isNull()){ //reset to default
    vx=m_defaultValue;
  }
  if(!Fact::setValue(vx))return false;
  save();
  return true;
}
void AppSettingFact::load()
{
  m_value=m_defaultValue;
  m_settings->beginGroup(static_cast<Fact*>(parentItem())->path());
  if((!m_settings->contains(name())) && (!m_defaultValue.isNull())){
    m_settings->setValue(name(),text());
  }
  Fact::setValue(m_settings->value(name(),m_defaultValue));
  m_settings->endGroup();
}
void AppSettingFact::save()
{
  m_settings->beginGroup(static_cast<Fact*>(parentItem())->path());
  m_settings->setValue(name(),text());
  m_settings->endGroup();
}
void AppSettingFact::loadSettings(const Fact *group)
{
  foreach(AppSettingFact *i,AppSettingFact::list){
    if(group){
      bool ok=false;
      for(const FactTree *ip=i->parentItem();ip;ip=ip->parentItem()){
        if(ip==group){
          ok=true;
          break;
        }
      }
      if(!ok)continue;
    }
    i->load();
  }
}
//=============================================================================
//=============================================================================

