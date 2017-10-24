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
#ifndef AppSettings_H
#define AppSettings_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
//=============================================================================
class AppSettings: public Fact
{
  Q_OBJECT
public:

  explicit AppSettings(FactSystem *parent=0);

  /*static AppSettings * instance()
  {
    return _instance;
  }*/

  static QSettings *settings()
  {
    return _instance->m_settings;
  }

  static QVariant value(const QString &namePath)
  {
    return _instance->findValue(namePath);
  }
  static bool setValue(QString namePath,const QVariant &v)
  {
    Fact *f=_instance->fact(namePath);
    if(!f)return false;
    return f->setValue(v);
  }

private:
  QSettings *m_settings;

  //static helpers
  static AppSettings * _instance;

};
//=============================================================================
class AppSettingFact: public Fact
{
  Q_OBJECT
public:
  explicit AppSettingFact(QSettings *settings,Fact *parent, QString name, QString label, QString descr, QString section, DataType dataType, QVariant defaultValue=QVariant());

  static QList<AppSettingFact*> list;
  static void loadSettings(const Fact *group);

  void load();
  void save();
private:
  QSettings *m_settings;
  QVariant m_defaultValue;
protected:
  //Fact override
  bool setValue(const QVariant &v);
};
//=============================================================================
#endif


