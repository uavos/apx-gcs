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
#ifndef AppSettingsPorts_H
#define AppSettingsPorts_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
class AppSettingsPorts;
//=============================================================================
class AppSettingsPort: public Fact
{
  Q_OBJECT
public:
  explicit AppSettingsPort(AppSettingsPorts *parent,const AppSettingsPort *port=NULL);

  Fact *_enabled;
  Fact *_type;
  Fact *_dev;
  Fact *_baud;
  Fact *_host;
  Fact *_save;
  Fact *_remove;

private:
  AppSettingsPorts *container;
  bool _new;

protected:
  QString name(void) const;
  QString title(void) const;
  QString descr(void) const;
private slots:
  void typeChanged();
public slots:
  void defaults();
};
//=============================================================================
class AppSettingsPorts: public Fact
{
  Q_OBJECT
public:
  explicit AppSettingsPorts(Fact *parent,const QString &sect=QString());

  QList<AppSettingsPort*> ports;

private:
  AppSettingsPort *_add;
  Fact *_allon;
  Fact *_alloff;

private slots:
  void allonTriggered();
  void alloffTriggered();

  void portsChanged();

public slots:
  void addTriggered();
  void removeTriggered();

  void load();
  void save();

};
//=============================================================================
#endif

