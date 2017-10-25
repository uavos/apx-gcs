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
#ifndef MANDALAFIELD_H
#define MANDALAFIELD_H
//-----------------------------------------------------------------------------
#include <QtCore>
class QMandalaItem;
//=============================================================================
class QMandalaField : public QObject
{
  Q_OBJECT
public:
  explicit QMandalaField();
  explicit QMandalaField(QMandalaItem *parent,uint var_msk);
  explicit QMandalaField(QMandalaItem *parent,uint var_msk,QString name,QString descr,QString units);
  Q_PROPERTY(double value READ value NOTIFY changed)
  Q_PROPERTY(QString name READ name NOTIFY params_changed)
  Q_PROPERTY(QString descr READ descr NOTIFY params_changed)
  Q_PROPERTY(QString units READ units NOTIFY params_changed)
  Q_PROPERTY(QString caption READ caption NOTIFY params_changed) //descr without units
  Q_PROPERTY(uint precision READ precision NOTIFY params_changed)
  Q_PROPERTY(QString text READ text NOTIFY changed)
  Q_CLASSINFO("DefaultProperty", "value")

  double value();
  QString name();
  QString descr();
  QString units();
  QString caption();
  uint type();
  uint varmsk();
  uint precision();
  QString text();

  QStringList enumStrings() const;
  //MandalaTreeField *treeField;

private:
  QMandalaItem *mvar;
  uint m_varmsk;
  double m_value;
  QString m_name,m_descr,m_units,m_caption,m_text;
  //var access
  uint m_type,m_precision;
  void *value_ptr;
  uint getPrecision();
  //set value behavior
  QTimer setValueUpdateTimer;
  void updateValue(double v);
  uint updateCnt;
  QTimer setValueDelayTimer;
  QTime setValueTime;
  double m_setValue;
  void setValue_do(double v);
  void setValue_done(double v);
  static char skipWriteFields[]; //indexes, which will skip setValue write to struct (stage)
  bool isSkipWriteField;
private slots:
  void updateMandalaPtr(QMandalaItem *m);
  void setValueDelayTimeout();
public slots:
  void update();
  Q_INVOKABLE void setValue(double v);
  Q_INVOKABLE void setValueLocal(double v);
  Q_INVOKABLE void request(void);
  Q_INVOKABLE void send(void);
signals:
  void changed();
  void params_changed();
};
//=============================================================================
#endif
