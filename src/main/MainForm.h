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
#ifndef MAINFORM_H
#define MAINFORM_H
#include <QtWidgets>
#include <QtNetwork>
#include "plugin_interface.h"
//=============================================================================
class DockWidget;
class QmlView;
class Vehicle;
class MainForm: public QMainWindow
{
  Q_OBJECT
public:
  MainForm(QWidget *parent = 0);

  void setPlugins(QStringList pfiles);

private:
  QStringList plugins_files;
  QHash<QString,PluginInterface*> plugins_so;
  QList<QString> plugins_qml;
  QMultiMap<QString,DockWidget*> plugins_docks;
  QMap<QDockWidget*,QmlView*> plugins_qmlviews;

  QMenu *mFile,*mEdit,*mTools,*mWindow,*mHelp;

  QMenu *mUAV;

  QTimer saveStateTimer;

  DockWidget *splashScreen;
  QLabel *loadingLabel;

  DockWidget *addDock(QString name,QWidget *w);

  QProcess vpnProcess;

  bool closing;
  bool loading;

protected:
  bool event(QEvent *event);
  void closeEvent(QCloseEvent *e);
private slots:
  void loadPlugins();
  void arrange();
  void lock();
  void unlock();
  void mFullScreen_triggered();
  void mVPN_triggered();

  void vpn_disconnected();

  void qmlPluginActionToggled(bool checked);

  void dockWidget_topLevelChanged(bool topLevel);
  void dockWidget_dockLocationChanged(Qt::DockWidgetArea area);
  void dockWidget_visibilityChanged(bool visible);
  void dockWidget_viewActionToggled(bool visible);

  void vehicleRegistered(Vehicle *v);
  void vehicleRemoved(Vehicle *v);
  void vehicleSelected(Vehicle *v);

public slots:
  void restoreGeometry();
  void saveDockState();
signals:
  void windowLoaded();
  void aboutToQuit();
};
//=============================================================================
class DockWidget: public QDockWidget
{
  Q_OBJECT
public:
  DockWidget(const QString & title, QWidget * parent):QDockWidget(title,parent){}
  void setDockSize(int w, int h);
private:
  QSize oldMaxSize, oldMinSize;
private slots:
  void returnToOldMaxMinSizes();
protected:
  void resizeEvent(QResizeEvent * event);
signals:
  void resized();
};
//=============================================================================
#endif
