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
#include "QmlApp.h"
#include <QMandala.h>
#include <QWidget>
#include <QSurface>
#include <QLayout>
#include <QAction>
#include <QPushButton>
#include <QGLFormat>
#include <QQuickStyle>
//=============================================================================
QmlApp::QmlApp(QString src, QObject *parent)
  : QQmlApplicationEngine(parent)
{
  qmlRegisterType<QMandalaItem>();
  qmlRegisterType<QMandala>();
  qmlRegisterType<QMandalaField>();
  qmlRegisterType<FlightDataFile>();
  //qmlRegisterType<DatalinkServer>();

  mandala=qApp->property("Mandala").value<QMandala*>();

  QQuickStyle::setStyle("Material");

  //if(QSettings().value("opengl",true).toBool()&&QGLFormat::openGLVersionFlags()){
    //setSurfaceType(QSurface::OpenGLSurface);
    /*QSurfaceFormat format;//(QSurfaceFormat::defaultFormat());
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    //format.setSwapInterval(1);
    setFormat(format);*/
  //}
  //setColor(QColor(Qt::black));
  //setClearBeforeRendering(true);
  //setFlags(Qt::FramelessWindowHint);
  //setResizeMode(QQuickView::SizeRootObjectToView);
  //setPersistentOpenGLContext(true);
  //setPersistentSceneGraph(true);

  //connect(mandala,SIGNAL(sizeChanged(uint)),this,SIGNAL(vehiclesChanged()));


  loadMandala(mandala->current);
  connect(mandala,SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(loadMandala(QMandalaItem*)));

  rootContext()->setContextProperty("font_narrow","Bebas Neue");
  rootContext()->setContextProperty("font_mono","FreeMono");
  rootContext()->setContextProperty("font_condenced","Ubuntu Condensed");

  rootContext()->setContextObject(this);

  if(src.isEmpty())return;

  QString sbase=src.left(src.lastIndexOf('/'));
  if(sbase.startsWith(':')||sbase.startsWith("qrc:")){
    removeImageProvider("svg");
    svgProvider = new SvgImageProvider(sbase.startsWith("qrc:")?sbase.mid(3):sbase);
    addImageProvider("svg", svgProvider);
    rootContext()->setContextProperty("svgRenderer", svgProvider);
    setBaseUrl(QUrl(sbase));
  }

  if(src.endsWith(".qml",Qt::CaseInsensitive))
    loadApp(src);
}
//=============================================================================
QQuickWindow *QmlApp::loadApp(QString file, QString title)
{
  rootContext()->setContextProperty("app_title",title);
  rootContext()->setContextProperty("app_source",file);
  load(QStringLiteral(":/AppWindow.qml"));
  qmlWindow = qobject_cast<QQuickWindow*>(rootObjects().first());
  //if(qmlWindow)return qmlWindow;
  //QQmlComponent component(this);
  //component.setData("ApplicationWindow { width: 1000; height: 600; visible: true; }", QUrl());
  //QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
  //rootObjects().first()->setParent(item);
  //item->setParentItem(this);

  //connect(qmlWindow,&QQuickWindow::sceneGraphInitialized,this,&QmlApp::resyncDraw);
  return qmlWindow;
}
//=============================================================================
QWidget *QmlApp::createWidget(QString title)
{
  rootContext()->setContextProperty("app_title",title);
  QWindow *qmlWindow = qobject_cast<QWindow*>(rootObjects().first());
  if(!qmlWindow)return NULL;
  w=QWidget::createWindowContainer(qmlWindow);
  //w->setAttribute(Qt::WA_OpaquePaintEvent);
  w->setAttribute(Qt::WA_NoSystemBackground);
  w->setWindowTitle(title);
  return w;
}
//=============================================================================
void QmlApp::loadMandala(QMandalaItem *mvar)
{
  foreach(QMandalaField *f,mvar->fields)
    rootContext()->setContextProperty(f->name(),f);
  foreach(QString key,mvar->constants.keys())
    rootContext()->setContextProperty(key,mvar->constants.value(key));

  rootContext()->setContextProperty("mandala",mandala);
}
//=============================================================================
void QmlApp::resyncDraw()
{
  if(!w->isVisible())return;
  /*QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();*/
  QQuickWindow *qmlWindow = qobject_cast<QQuickWindow*>(rootObjects().first());
  QWindow *pw=qmlWindow->parent();
  if(!(pw&&pw->winId()))return;
  qmlWindow->hide();
  qmlWindow->setParent(0);
  qmlWindow->setParent(pw);
  qmlWindow->show();
  //blockShowEvt=false;
}
//=============================================================================
