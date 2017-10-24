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
//#ifdef OLDLINUX
//#define FIX_WINDOWID
//#endif
//=============================================================================
QmlView::QmlView(QString src,QWindow *parent)
  : QQuickView(parent),menu(NULL),blockShowEvt(true)
{
  qmlRegisterType<QMandalaItem>();
  qmlRegisterType<QMandala>();
  qmlRegisterType<QMandalaField>();
  qmlRegisterType<FlightDataFile>();

  mandala=qApp->property("Mandala").value<QMandala*>();

  //QQuickStyle::setStyle("Material");

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

  connect(mandala,SIGNAL(sizeChanged(uint)),this,SIGNAL(vehiclesChanged()));


  loadMandala(mandala->current);
  connect(mandala,SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(loadMandala(QMandalaItem*)));

  QQmlEngine *e=engine();
  e->rootContext()->setContextProperty("font_narrow","Bebas Neue");
  e->rootContext()->setContextProperty("font_mono","FreeMono");
  e->rootContext()->setContextProperty("font_condenced","Ubuntu Condensed");

  e->rootContext()->setContextProperty("settings",settings);
  e->rootContext()->setContextProperty("actions",QVariant::fromValue(actions));
  //e->rootContext()->setContextObject(this);

  //add app object
  QJSValue jsApp=e->newQObject(qApp);
  e->globalObject().setProperty("xapp",jsApp);
  QQmlEngine::setObjectOwnership(qApp,QQmlEngine::CppOwnership);
  //add all app child QObjects
  foreach(QString s,qApp->dynamicPropertyNames()){
    QVariant v=qApp->property(s.toUtf8().data());
    //qDebug()<<s<<v;
    if(v.canConvert(QMetaType::QObjectStar)){
      //qDebug()<<s;
      jsApp.setProperty(s,e->newQObject(v.value<QObject*>()));
      QQmlEngine::setObjectOwnership(v.value<QObject*>(),QQmlEngine::CppOwnership);
    }
  }

  FactSystem::instance()->syncJS(e);



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

#ifdef FIX_WINDOWID
  hide();
  QTimer::singleShot(50,this,SLOT(resyncDraw()));
#endif
  return w;
}
//=============================================================================
void QmlView::loadMandala(QMandalaItem *mvar)
{
  QQmlEngine *e=engine();
  foreach(QMandalaField *f,mvar->fields)
    e->rootContext()->setContextProperty(f->name(),f);
  foreach(QString key,mvar->constants.keys())
    e->rootContext()->setContextProperty(key,mvar->constants.value(key));

  e->rootContext()->setContextProperty("mandala",mandala);
}
//=============================================================================
void QmlView::showEvent(QShowEvent *e)
{
  QQuickView::showEvent(e);
  //QML tabbed dock widget hide workaround
  //qDebug()<<"showEvent"<<e;
#ifdef FIX_WINDOWID
  if(blockShowEvt)return;
  blockShowEvt=true;
  resyncDraw();
#endif
}
//=============================================================================
void QmlView::resyncDraw()
{
  if(!w->isVisible())return;
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QWindow *pw=parent();
  if(!(pw&&pw->winId()))return;
  hide();
  setParent(0);
  setParent(pw);
  show();
  blockShowEvt=false;
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
//=============================================================================
QQmlListProperty<QMandalaItem> QmlView::vehicles()
{
  return QQmlListProperty<QMandalaItem>(mandala,mandala->items);
}
//=============================================================================

