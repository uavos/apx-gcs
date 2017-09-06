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
#include "CompassPlugin.h"
#include "QMandala.h"
//=============================================================================
void CompassPlugin::init(void)
{
  fobj=NULL;
  dock=NULL;
  aShow = new QAction(QIcon(":/icons/old/bt.png"),tr("Compass Calibration"), this);
  connect(aShow, SIGNAL(triggered()), this, SLOT(showDlg()));
  obj = aShow;
}
//=============================================================================
void CompassPlugin::showDlg(void)
{
  if(dock){
    dock->show();
    dock->raise();
    return;
  }
  fobj = new CompassFrame();
  connect(fobj, SIGNAL(closed()), this, SLOT(dlgClosed()));
  QWidget* m = NULL;
  foreach(QWidget *widget, qApp->topLevelWidgets())
    if(widget->inherits("QMainWindow")){
      m=widget;
      break;
    }
  dock = new QDockWidget(fobj->windowTitle(),m);
  dock->installEventFilter( this );

  //connect(dock, SIGNAL(topLevelChanged(bool)), this, SLOT(dockWidget_topLevelChanged(bool)));
  //connect(dock, SIGNAL(visibilityChanged(bool)), this, SLOT(dockWidget_topLevelChanged(bool)));
  //dock->setFeatures(QDockWidget::DockWidgetClosable);
  dock->setAllowedAreas(Qt::NoDockWidgetArea);
  dock->setAttribute(Qt::WA_MacAlwaysShowToolWindow,true);
  dock->setObjectName(name);
  dock->setWidget(fobj);
  dock->setFloating(true);
  dock->restoreGeometry(QSettings().value(name+"State").toByteArray());
  dock->show();
  //fobj->show();
}
//=============================================================================
void CompassPlugin::dockWidget_topLevelChanged(bool isFloating)
{
  Q_UNUSED(isFloating)
  /*QDockWidget *dock=(QDockWidget*)sender();
  if(isFloating){
    dock->setWindowFlags(Qt::ToolTip|Qt::WindowTitleHint);
    dock->show();
  }//else dock->setFloating(true);*/
  //if(!isFloating) qDebug("closed");
    //dlgClosed();
}
//=============================================================================
bool CompassPlugin::eventFilter(QObject *obj, QEvent *event)
{
  if(event->type()==QEvent::Close){
    //qDebug("closed");
    dlgClosed();
    return true;
  }
  return QObject::eventFilter( obj, event );
}
//=============================================================================
void CompassPlugin::dlgClosed()
{
  QSettings().setValue(name+"State",dock->saveGeometry());
  fobj->deleteLater();
  dock->deleteLater();
  fobj=NULL;
  dock=NULL;
}
//=============================================================================
