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
#ifndef QmlView_H
#define QmlView_H
#include <QtQuick>
#include <QAbstractItemModel>
#include <QMenu>
#include "SvgImageProvider.h"
//=============================================================================
class QMLSettings : public QSettings
{
  Q_OBJECT
public:
  explicit QMLSettings(QObject *parent = 0) : QSettings(parent) {}
  Q_INVOKABLE inline void setValue(const QString &key, const QVariant &value) { QSettings::setValue(key, value); }
  Q_INVOKABLE inline QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const
  {
    QVariant value = QSettings::value(key, defaultValue);
    if(QString(value.typeName())=="QString"&&(value.toString()=="false"||value.toString()=="true"))
      return QVariant(value.toBool());
    return value;
  }
};
Q_DECLARE_METATYPE(QMLSettings*)
//=============================================================================
class QmlMenu : public QMenu
{
  Q_OBJECT
public:
  explicit QmlMenu(QWidget *parent);
  Q_INVOKABLE QString exec(int x, int y);
};
//=============================================================================
class QmlView : public QQuickView
{
  Q_OBJECT
public:
  explicit QmlView(QString src,QWindow * parent = 0);

  QWidget *createWidget(QString title=QString());

  void addAction(QAction *a);
  QAction * addAction(const QIcon & icon, const QString & text, const QObject * receiver, const char * member);

  QMenu *menu;
  QMLSettings *settings;
  QList<QObject*> actions;

private:
  SvgImageProvider *svgProvider;
  bool blockShowEvt;
  QWidget *w;
protected:
  void mousePressEvent(QMouseEvent *e);

signals:
  void customContextMenuRequested(const QPoint & pos);

};
//=============================================================================
#endif
