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
#include "QmlView.h"
#include <QMandala.h>
#include <QWidget>
#include <QSurface>
#include <QLayout>
#include <QAction>
#include <QPushButton>
#include <QGLFormat>
#include <QQuickStyle>
#include "FactSystem.h"
//=============================================================================
QmlView::QmlView(QString src,QWindow *parent)
  : QQuickView(FactSystem::instance()->engine(),parent),menu(NULL),blockShowEvt(true)
{

  settings=new QMLSettings();

  //if(QSettings().value("opengl",true).toBool()&&QGLFormat::openGLVersionFlags()){
    //setSurfaceType(QSurface::OpenGLSurface);
    /*QSurfaceFormat format;//(QSurfaceFormat::defaultFormat());
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    //format.setSwapInterval(1);
    setFormat(format);*/
  //}
  setColor(QColor(Qt::black));
  //setClearBeforeRendering(true);
  //setFlags(Qt::FramelessWindowHint);
  setResizeMode(QQuickView::SizeRootObjectToView);
  //setPersistentOpenGLContext(true);
  //setPersistentSceneGraph(true);

  QQmlEngine *e=engine();
  e->rootContext()->setContextProperty("settings",settings);
  e->rootContext()->setContextProperty("actions",QVariant::fromValue(actions));

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
    setSource(QUrl(src));
}
//=============================================================================
void QmlView::addAction(QAction *a)
{
  actions.append(a);
  engine()->rootContext()->setContextProperty("actions",QVariant::fromValue(actions));
}
QAction * QmlView::addAction(const QIcon & icon, const QString & text, const QObject * receiver, const char * member)
{
  QAction *a=new QAction(this);
  a->setIcon(icon);
  a->setText(text);
  connect(a,SIGNAL(triggered(bool)),receiver,member);
  addAction(a);
  return a;
}
//=============================================================================
QWidget *QmlView::createWidget(QString title)
{
  w=QWidget::createWindowContainer(this);
  w->setAttribute(Qt::WA_OpaquePaintEvent);
  w->setAttribute(Qt::WA_NoSystemBackground);
  w->setWindowTitle(title);

  menu=new QMenu(w);
  return w;
}
//=============================================================================
void QmlView::mousePressEvent(QMouseEvent *e)
{
  QQuickView::mousePressEvent(e);
  if(!menu)return;
  if(e->button()==Qt::RightButton){
    emit customContextMenuRequested(e->pos());
    if(!menu->isEmpty())
      menu->popup(mapToGlobal(e->pos()));
  }
}
//=============================================================================
//=============================================================================
QmlMenu::QmlMenu(QWidget *parent)
 : QMenu(parent)
{
}
//=============================================================================
QString QmlMenu::exec(int x, int y)
{
  QAction *a=QMenu::exec(mapToGlobal(QPoint(x, y)),0);
  QString text;
  if(a) text=a->text();
  return text;
}
//=============================================================================

