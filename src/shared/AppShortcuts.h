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
class AppShortcuts;
//=============================================================================
class AppShortcut : public QObject
{
  Q_OBJECT
public:
  AppShortcut(AppShortcuts *parent,QWidget *widget);
  ~AppShortcut();

  void updateShortcut();

private:
  AppShortcuts *appShortcuts;
  QWidget *widget;
  QShortcut *shortcut;
  QMandala *mandala;
private slots:
  void activated();
public:
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)
  Q_PROPERTY(QString cmd READ cmd WRITE setCmd NOTIFY cmdChanged)
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

  QString name() const;
  void setName(const QString &);
  QString key() const;
  void setKey(const QString &);
  QString cmd() const;
  void setCmd(const QString &);
  bool enabled() const;
  void setEnabled(const bool);
  bool valid() const;
  void setValid(const bool);
private:
  QString m_name;
  QString m_key;
  QString m_cmd;
  bool m_enabled;
  bool m_valid;
signals:
  void nameChanged();
  void keyChanged();
  void cmdChanged();
  void enabledChanged();
  void validChanged();
};
//=============================================================================
class AppShortcutModel : public QAbstractListModel
{
  Q_OBJECT
public:
  enum AppShortcutRoles {
      ItemRole = Qt::UserRole + 1
  };
  AppShortcutModel(QObject *parent = 0);
  Q_INVOKABLE void addItem(AppShortcut *item);
  Q_INVOKABLE void removeItem(AppShortcut *item);
  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

  AppShortcut *item(int row);
protected:
  QHash<int, QByteArray> roleNames() const;
private:
  QList<AppShortcut*> m_items;
signals:
  void changed();
};
//=============================================================================
class AppShortcuts : public QObject
{
  Q_OBJECT
public:
  AppShortcuts(QWidget *parent);

  Q_INVOKABLE void enableAllSystem(bool v);
  Q_INVOKABLE void enableAllUser(bool v);

  Q_INVOKABLE void addNew();

  Q_INVOKABLE void load();
  Q_INVOKABLE void save();

  Q_INVOKABLE QString keyToPortableString(int key,int modifier) const;

private:
  QWidget *widget;
  QMandala *mandala;

  //PROPERTIES
public:
  Q_PROPERTY(AppShortcutModel *systemShortcuts READ systemShortcuts NOTIFY shortcutsChanged)
  AppShortcutModel *systemShortcuts();
  Q_PROPERTY(AppShortcutModel *userShortcuts READ userShortcuts NOTIFY shortcutsChanged)
  AppShortcutModel *userShortcuts();

  Q_PROPERTY(AppShortcut *newItem READ newItem NOTIFY shortcutsChanged)
  AppShortcut *newItem() const;

  Q_PROPERTY(bool blocked READ blocked WRITE setBlocked NOTIFY blockedChanged)
  bool blocked() const;
  void setBlocked(bool v);

private:
  AppShortcut *m_newItem;
  AppShortcutModel m_systemShortcuts;
  AppShortcutModel m_userShortcuts;
  bool m_blocked;
signals:
  void shortcutsChanged();
  void blockedChanged();
};
//=============================================================================
#endif
