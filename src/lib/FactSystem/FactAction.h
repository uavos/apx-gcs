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
#ifndef FactAction_H
#define FactAction_H
//=============================================================================
#include <QtCore>
class Fact;
//=============================================================================
class FactAction: public QObject
{
  Q_OBJECT
  Q_ENUMS(FactActionType)

  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(QString title READ title NOTIFY titleChanged)
  Q_PROPERTY(QString descr READ descr NOTIFY descrChanged)
  Q_PROPERTY(QString icon READ icon NOTIFY iconChanged)
  Q_PROPERTY(int flags READ flags CONSTANT)
  Q_PROPERTY(FactActionType actionType READ actionType CONSTANT)

  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
  enum FactActionType {
    NormalAction =0,
    ApplyAction,
    RemoveAction,
  };
  Q_ENUM(FactActionType)

  explicit FactAction(Fact *parent, const QString &name, const QString &title, const QString &descr, FactActionType actionType, const QString &icon=QString());
  explicit FactAction(Fact *parent, FactAction *linkAction);

  virtual QVariant data(int col, int role) const;

private:
  Fact *fact;
  FactAction *linkAction;

public slots:
  virtual void trigger(void);

signals:
  void triggered();


  //-----------------------------------------
  //PROPERTIES
public:
  QString name(void) const;
  void setName(const QString &v);
  QString title(void) const;
  void setTitle(const QString &v);
  QString descr(void) const;
  void setDescr(const QString &v);
  QString icon(void) const;
  void setIcon(const QString &v);

  int flags() const;
  FactActionType actionType() const;

  bool enabled() const;
  void setEnabled(const bool &v);
  bool visible() const;
  void setVisible(const bool &v);

protected:
  QString  m_name;
  QString  m_title;
  QString  m_descr;
  QString  m_icon;
  int m_flags;
  FactActionType m_actionType;
  bool m_enabled;
  bool m_visible;

signals:
  void nameChanged();
  void titleChanged();
  void descrChanged();
  void iconChanged();
  void enabledChanged();
  void visibleChanged();

};
//=============================================================================
#endif
