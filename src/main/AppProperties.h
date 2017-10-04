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
#ifndef AppProperties_H
#define AppProperties_H
//=============================================================================
#include <QtCore>
//=============================================================================
//directories
class AppDirs: public QObject
{
  Q_OBJECT
public:
  explicit AppDirs(QObject *parent=0) : QObject(parent) {}

  static QDir user();         // local user files (Documents)
  static QDir res();          // resources
  static QDir plugins();      // plugins
  static QDir userPlugins();  // user plugins
  static QDir telemetry();    // saved flight data
  static QDir maps();         // maps and tilesets
  static QDir lang();         // translations
  static QDir missions();     // saved flight plans
  static QDir configs();      // user saved nodes config files
  static QDir nodes();        // nodes backups
  static QDir scripts();      // user saved scripts
};
//=============================================================================
class AppProperties: public QSettings
{
  Q_OBJECT
public:

  explicit AppProperties(QObject *parent=0);


  // QML QSettings wrapper
  Q_INVOKABLE void setValue(const QString &grp, const QString &key, const QVariant &value);
  Q_INVOKABLE QVariant value(const QString &grp, const QString &key, const QVariant &defaultValue = QVariant());

  //PROPERTIES
public:
  //app identity
  Q_PROPERTY(QString version READ version CONSTANT) // app version
  QString version();

  Q_PROPERTY(QString branch READ branch CONSTANT) // app repo branch
  QString branch();

  Q_PROPERTY(bool devMode READ devMode CONSTANT) // development run (i.e. from user home folder)
  bool devMode();


  //global properties
  Q_PROPERTY(bool jsw READ jsw WRITE setJsw NOTIFY jswChanged)
  bool jsw();


  //settings
  Q_PROPERTY(bool soundsEnabled READ soundsEnabled WRITE setSoundsEnabled NOTIFY soundsEnabledChanged)
  bool soundsEnabled();
  Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
  bool readOnly();
  Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
  bool smooth();
  Q_PROPERTY(bool test READ test WRITE setTest NOTIFY testChanged)
  bool test();


private:
  bool m_soundsEnabled;
  bool m_readOnly;
  bool m_smooth;
  bool m_test;

  bool m_jsw;
public slots:
  Q_INVOKABLE void setSoundsEnabled(bool v);
  Q_INVOKABLE void setReadOnly(bool v);
  Q_INVOKABLE void setSmooth(bool v);
  Q_INVOKABLE void setTest(bool v);
  Q_INVOKABLE void setJsw(bool v);

signals:
  void soundsEnabledChanged(bool);
  void readOnlyChanged(bool);
  void smoothChanged(bool);
  void testChanged(bool);
  void jswChanged(bool);
};
Q_DECLARE_METATYPE(AppProperties*)
//=============================================================================
#endif
