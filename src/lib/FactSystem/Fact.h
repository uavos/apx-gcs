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
#include "FactListModel.h"
class FactSystem;
//=============================================================================
class Fact: public FactData
{
  Q_OBJECT

  Q_PROPERTY(FactListModel * model READ model CONSTANT)

  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

  Q_PROPERTY(QString section READ section WRITE setSection NOTIFY sectionChanged)
  Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
  Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
  Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)

  Q_PROPERTY(QString iconSource READ iconSource NOTIFY iconSourceChanged)

  Q_PROPERTY(QString qmlMenu READ qmlMenu NOTIFY qmlMenuChanged)
  Q_PROPERTY(QString qmlEditor READ qmlEditor NOTIFY qmlEditorChanged)

  Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)

public:
  explicit Fact(FactTree *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType);

  Q_INVOKABLE QByteArray hash() const;

  Q_INVOKABLE QVariant findValue(const QString &namePath);

  Q_INVOKABLE Fact * fact(const QString &factName) const;
  Q_INVOKABLE Fact * childFact(int i) const;
  Q_INVOKABLE Fact * childByTitle(const QString &factTitle) const;
  //Q_INVOKABLE Fact * byPath(const QString &itemNamePath) const;

  Q_INVOKABLE QString titlePath(const QChar pathDelimiter=QChar('/')) const;


  virtual void bind(FactData *item);

  QVariant userData;

  virtual bool lessThan(Fact *rightFact) const; //sorting helper

  //data model
  enum {
    FACT_MODEL_COLUMN_NAME=0,
    FACT_MODEL_COLUMN_VALUE,
    FACT_MODEL_COLUMN_DESCR,

    FACT_MODEL_COLUMN_CNT,
  };
  enum FactModelRoles {
    ModelDataRole = Qt::UserRole + 1,
    NameRole,
    ValueRole,
    TextRole,
  };
  virtual QVariant data(int col, int role) const;

protected:
  virtual void hashData(QCryptographicHash *h) const;


public slots:
  virtual void trigger(void); //execute fact event (onClick)
signals:
  void triggered();

public:
  //---------------------------------------
  FactListModel * model() const;

  bool enabled() const;
  void setEnabled(const bool &v);

  bool visible() const;
  void setVisible(const bool &v);

  QString section() const;
  void setSection(const QString &v);

  QString status() const;
  void setStatus(const QString &v);

  bool active() const;
  void setActive(const bool &v);

  int progress() const;
  void setProgress(const int &v);

  QString iconSource() const;
  void setIconSource(const QString &v);

  QString qmlMenu() const;
  void setQmlMenu(const QString &v);
  QString qmlEditor() const;
  void setQmlEditor(const QString &v);

  bool busy() const;
  void setBusy(const bool &v);

protected:
  FactListModel *m_model;
  bool m_enabled;
  bool m_visible;
  QString  m_section;
  QString  m_status;
  bool m_active;
  int m_progress;
  QString  m_iconSource;
  QString  m_qmlMenu;
  QString  m_qmlEditor;
  bool m_busy;

signals:
  void enabledChanged();
  void visibleChanged();

  void sectionChanged();
  void statusChanged();
  void activeChanged();
  void progressChanged();

  void iconSourceChanged();
  void qmlMenuChanged();
  void qmlEditorChanged();
  void busyChanged();

};
//=============================================================================
#endif
