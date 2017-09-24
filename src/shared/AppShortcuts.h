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
#ifndef AppShortcuts_H
#define AppShortcuts_H
#include <QtCore>
#include <QShortcut>
#include <QQmlListProperty>
class QMandala;
//=============================================================================
class AppShortcut : public QObject
{
  Q_OBJECT
public:
  AppShortcut(QObject *parent = 0);
  ~AppShortcut();

  void updateShortcut(QWidget *widget);
private:
  QShortcut *shortcut;
  QMandala *mandala;
private slots:
  void activated();
public:
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
  Q_PROPERTY(QString key READ key WRITE setKey NOTIFY changed)
  Q_PROPERTY(QString cmd READ cmd WRITE setCmd NOTIFY changed)

  QString name() const;
  void setName(const QString &);
  QString key() const;
  void setKey(const QString &);
  QString cmd() const;
  void setCmd(const QString &);
private:
  QString m_name;
  QString m_key;
  QString m_cmd;
signals:
  void changed();
};
//=============================================================================
class AppShortcuts : public QObject
{
  Q_OBJECT
public:
  AppShortcuts(QWidget *parent);

  QVector<AppShortcut *> shortcuts;

private:
  QWidget *widget;
  QMandala *mandala;

  void load();
  void save();

public slots:
signals:
  //void exec(QString);

  //PROPERTIES
public:
  Q_PROPERTY(QQmlListProperty<AppShortcut> items READ items NOTIFY itemsChanged)
  QQmlListProperty<AppShortcut> items();

private:
    static void appendItem(QQmlListProperty<AppShortcut>*, AppShortcut*);
    static int itemCount(QQmlListProperty<AppShortcut>*);
    static AppShortcut* item(QQmlListProperty<AppShortcut>*, int);
    static void clearItems(QQmlListProperty<AppShortcut>*);
signals:
    void itemsChanged();
};
//=============================================================================
#endif
