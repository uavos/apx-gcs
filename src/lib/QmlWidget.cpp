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
#include <QtQuick>
#include "QmlWidget.h"
#include <QMandala.h>
#include <QWidget>
#include <QSurface>
#include <QLayout>
#include <QAction>
#include <QPushButton>
#include <QGLFormat>
#include "DatalinkServer.h"
#include <QQuickStyle>
//=============================================================================
QmlWidget::QmlWidget(QString src, QWidget *parent)
  : QQuickWidget(parent)
{
  qmlRegisterType<QMandalaItem>();
  qmlRegisterType<QMandala>();
  qmlRegisterType<QMandalaField>();
  qmlRegisterType<FlightDataFile>();
  qmlRegisterType<DatalinkServer>();

  mandala=qApp->property("Mandala").value<QMandala*>();

  QQuickStyle::setStyle("Material");

  if(!QSettings().contains("smooth_instruments"))
    QSettings().setValue("smooth_instruments",true);

  resize(200,100);
  //setMinimumHeight(100);
  //setMinimumWidth(100);

  //if(QSettings().value("opengl",true).toBool()&&QGLFormat::openGLVersionFlags()){
    //setSurfaceType(QSurface::OpenGLSurface);
    /*QSurfaceFormat format;//(QSurfaceFormat::defaultFormat());
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    //format.setSwapInterval(1);
    setFormat(format);*/
  //}
  quickWindow()->setColor(QColor(Qt::black));
  //quickWindow()->setClearBeforeRendering(true);
  //quickWindow()->setFlags(Qt::FramelessWindowHint);
  setResizeMode(QQuickWidget::SizeRootObjectToView);
  //quickWindow()->setPersistentOpenGLContext(true);
  //quickWindow()->setPersistentSceneGraph(true);


  loadMandala(mandala->current);
  connect(mandala,SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(loadMandala(QMandalaItem*)));

  QQmlEngine *e=engine();
  rootContext()->setContextProperty("font_narrow","Bebas Neue");
  rootContext()->setContextProperty("font_mono","FreeMono");
  rootContext()->setContextProperty("font_condenced","Ubuntu Condensed");

  if(src.isEmpty())return;

  QString sbase=src.left(src.lastIndexOf('/'));
  if(sbase.startsWith(':')||sbase.startsWith("qrc:")){
    e->removeImageProvider("svg");
    svgProvider = new SvgImageProvider(sbase.startsWith("qrc:")?sbase.mid(3):sbase);
    e->addImageProvider("svg", svgProvider);
    e->rootContext()->setContextProperty("svgRenderer", svgProvider);
    e->setBaseUrl(QUrl(sbase));
  }

  if(src.endsWith(".qml",Qt::CaseInsensitive))
    loadApp(src);
}
//=============================================================================
void QmlWidget::loadApp(QString file)
{
  setSource(QUrl(file));

  /*QQmlEngine *e=engine();
  e->rootContext()->setContextProperty("app_title",file);
  e->rootContext()->setContextProperty("app_source",file);
  setSource(QStringLiteral("AppWindow.qml"));*/
}
//=============================================================================
void QmlWidget::loadMandala(QMandalaItem *mvar)
{
  QQmlEngine *e=engine();
  foreach(QMandalaField *f,mvar->fields)
    e->rootContext()->setContextProperty(f->name(),f);
  foreach(QString key,mvar->constants.keys())
    e->rootContext()->setContextProperty(key,mvar->constants.value(key));

  e->rootContext()->setContextProperty("mandala",mandala);
}
//=============================================================================
