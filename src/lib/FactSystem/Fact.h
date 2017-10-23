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
#ifndef Fact_H
#define Fact_H
//=============================================================================
#include "FactData.h"
//=============================================================================
class Fact: public FactData
{
  Q_OBJECT

  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

  Q_PROPERTY(QString section READ section WRITE setSection NOTIFY sectionChanged)
  Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)

public:
  explicit Fact(FactTree *parent, QString name, QString title, QString descr, ItemType treeItemType, DataType dataType);

  Q_INVOKABLE QString path(int fromLevel=0,const QChar pathDelimiter=QChar('.')) const;

  Q_INVOKABLE Fact * fact(const QString &factName) const;
  //Q_INVOKABLE Fact * byPath(const QString &itemNamePath) const;

public slots:
  virtual void trigger(void); //execute fact event (onClick)
signals:
  void triggered();

public:
  //---------------------------------------
  virtual bool enabled() const;
  virtual void setEnabled(const bool &v);

  virtual bool visible() const;
  virtual void setVisible(const bool &v);

  virtual QString section() const;
  virtual void setSection(const QString &v);

  virtual QString status() const;
  virtual void setStatus(const QString &v);

protected:
  bool m_enabled;
  bool m_visible;
  QString  m_section;
  QString  m_status;

signals:
  void enabledChanged();
  void visibleChanged();

  void sectionChanged();
  void statusChanged();

};
//=============================================================================
#endif
