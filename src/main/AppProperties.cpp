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
#include "AppProperties.h"
//=============================================================================
AppProperties::AppProperties(QObject *parent)
 : QSettings(parent)
{
  setObjectName("prefs");

  m_soundsEnabled=false;
  setSoundsEnabled(QSettings().value("sounds",false).toBool());
  m_readOnly=false;
  setReadOnly(QSettings().value("readOnly",false).toBool());
  m_smooth=true;
  setSmooth(QSettings().value("smooth",true).toBool());
  m_test=false;
  m_jsw=false;
}
//=============================================================================
// QML QSettings wrapper
void AppProperties::setValue(const QString &grp, const QString &key, const QVariant &value)
{
  QSettings::beginGroup(grp);
  QSettings::setValue(key, value);
  QSettings::endGroup();
}
QVariant AppProperties::value(const QString &grp, const QString &key, const QVariant &defaultValue)
{
  QSettings::beginGroup(grp);
  QVariant value = QSettings::value(key, defaultValue);
  if(QString(value.typeName())=="QString"&&(value.toString()=="false"||value.toString()=="true"))
    value=QVariant(value.toBool());
  QSettings::endGroup();
  return value;
}
//=============================================================================
// PROPERTIES
//=============================================================================
#define VSTR_IMPL(a) #a
#define VSTR(a) VSTR_IMPL(a)
QString AppProperties::version()
{
  QString s=VSTR(VERSION);
  if(s.isEmpty())return "unknown version";
  return s;
}
QString AppProperties::branch()
{
  return VSTR(BRANCH);
}
bool AppProperties::devMode()
{
#ifdef __ANDROID__
  return false;
#else
  return QCoreApplication::applicationDirPath().contains(QDir::homePath());
#endif
}
//=============================================================================
bool AppProperties::soundsEnabled()
{
  return m_soundsEnabled;
}
void AppProperties::setSoundsEnabled(bool v)
{
  if(m_soundsEnabled==v)return;
  m_soundsEnabled=v;
  QSettings().setValue("sounds",v);
  emit soundsEnabledChanged(v);
}
bool AppProperties::readOnly()
{
  return m_readOnly;
}
void AppProperties::setReadOnly(bool v)
{
  if(m_readOnly==v)return;
  m_readOnly=v;
  QSettings().setValue("readOnly",v);
  emit readOnlyChanged(v);
  if(v)qDebug("%s",tr("Read only datalink").toUtf8().data());
  else qDebug("%s",tr("Uplink allowed").toUtf8().data());
}
bool AppProperties::smooth()
{
  return m_smooth;
}
void AppProperties::setSmooth(bool v)
{
  if(m_smooth==v)return;
  m_smooth=v;
  QSettings().setValue("smooth",v);
  emit smoothChanged(v);
}
bool AppProperties::test()
{
  return m_test;
}
void AppProperties::setTest(bool v)
{
  if(m_test==v)return;
  m_test=v;
  emit testChanged(v);
}
bool AppProperties::jsw()
{
  return m_jsw;
}
void AppProperties::setJsw(bool v)
{
  if(m_jsw==v)return;
  m_jsw=v;
  emit jswChanged(v);
  if(v)qDebug("%s",tr("Joystick available").toUtf8().data());
  else qWarning("%s",tr("Joystick off").toUtf8().data());
}
//=============================================================================
//=============================================================================
QDir AppDirs::plugins()
{
  return QDir(QCoreApplication::applicationDirPath()+"/../plugins/gcs");
}
QDir AppDirs::res()
{
#ifdef __ANDROID__
  const QString hpath=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  return QDir("assets:/data/");
#else
  return QDir(QCoreApplication::applicationDirPath()+"/../resources");
#endif
}
QDir AppDirs::user()
{
#ifdef __ANDROID__
  return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/.gcu");
#else
  return QDir(QDir(
    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    .absoluteFilePath("UAVOS") );
#endif
}
QDir AppDirs::telemetry()
{
  return QDir(user().absoluteFilePath("Telemetry"));
}
QDir AppDirs::maps()
{
  return QDir(user().absoluteFilePath("Maps"));
}
QDir AppDirs::lang()
{
  return QDir(QCoreApplication::applicationDirPath()+"/../localization/gcs");
}
QDir AppDirs::missions()
{
  return QDir(user().absoluteFilePath("Missions"));
}
QDir AppDirs::configs()
{
  return QDir(user().absoluteFilePath("Configs"));
}
QDir AppDirs::nodes()
{
  return QDir(user().absoluteFilePath("Nodes"));
}
QDir AppDirs::scripts()
{
  return QDir(user().absoluteFilePath("Scripts"));
}
QDir AppDirs::userPlugins()
{
  return QDir(user().absoluteFilePath("Plugins"));
}
//=============================================================================
//=============================================================================

