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
#include "SvgMaterialIcon.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QApplication>
#include <QPalette>
#include <QFontDatabase>
#include <FactSystem.h>
//=============================================================================
QHash<QString,QChar> SvgMaterialIcon::map;
QVariantMap SvgMaterialIcon::qmlMap;
SvgMaterialIcon::SvgMaterialIcon(const QString &name, const QColor &color)
  : QIcon(icon(name,color))
{
  //https://materialdesignicons.com/
}
//=============================================================================
QIcon SvgMaterialIcon::icon(const QString &name,const QColor &color) const
{
  QChar c=getChar(name);
  if(c.isNull()){
    return QIcon();
  }
  QFontIconEngine * engine = new QFontIconEngine;
  engine->setFontFamily("Material Design Icons");
  engine->setLetter(c);
  engine->setBaseColor(color);
  return QIcon(engine);
}
//=============================================================================
//=============================================================================
void SvgMaterialIcon::initialize()
{
  updateMap();
  QHashIterator<QString,QChar> i(map);
  while (i.hasNext()) {
      i.next();
      qmlMap[i.key()]=QString(i.value());//.toLatin1();
  }
  QQmlEngine *e=FactSystem::instance()->engine();
  e->globalObject().setProperty("materialIconChar",e->toScriptValue(qmlMap));
  //e->rootContext()->setContextProperty("materialIconChar",qmlMap);
}
//=============================================================================
QChar SvgMaterialIcon::getChar(const QString &name)
{
  if(map.isEmpty())updateMap();
  if(!map.contains(name)){
    qWarning("Material icon is missing: %s",name.toUtf8().data());
    return QChar();
  }
  return map.value(name);
}
//=============================================================================
void SvgMaterialIcon::updateMap()
{
  QFile res;
  res.setFileName(":/icons/material-icons.ttf");
  if(res.open(QIODevice::ReadOnly)){
    QFontDatabase::addApplicationFontFromData(res.readAll());
    res.close();
  }
  res.setFileName(":/icons/material-icons.css");
  QStringList st;
  if(res.open(QIODevice::ReadOnly)){
    QString s=res.readAll();
    st=s.split(".mdi-");
    res.close();
  }
  if(st.isEmpty()){
    qWarning("Material icons CSS is missing");
    return;
  }
  for(int i=0;i<st.size();++i){
    QString s=st.at(i);
    int pos=s.indexOf(":before");
    if(pos<1)continue;
    QString sname=s.left(pos).trimmed();
    s=s.mid(pos+1);
    pos=s.indexOf("content:");
    if(pos<1)continue;
    s=s.mid(pos);
    s=s.mid(s.indexOf(':')+1);
    s=s.left(s.indexOf(';')).trimmed().remove('"').replace("\\","\\u");
    QRegExp rx("(\\\\u[0-9a-fA-F]{4})");
    if(rx.indexIn(s) != -1) {
      map[sname]=QChar(rx.cap(1).right(4).toUShort(0, 16));
    }
  }
  //qDebug()<<map;
}
//=============================================================================
//=============================================================================
QFontIconEngine::QFontIconEngine()
  :QIconEngine()
{
}
QFontIconEngine::~QFontIconEngine()
{
}
void QFontIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
  Q_UNUSED(state);
  QFont font = QFont(mFontFamily);
  int drawSize = rect.height();//qRound(rect.height() * 0.8);
  font.setPixelSize(drawSize>1?drawSize:1);

  QColor penColor;
  if (!mBaseColor.isValid())
    penColor = QApplication::palette("QWidget").color(QPalette::Normal,QPalette::ButtonText);
  else
    penColor = mBaseColor;

  if (mode == QIcon::Disabled)
    penColor = QApplication::palette("QWidget").color(QPalette::Disabled,QPalette::ButtonText);


  if (mode == QIcon::Selected)
    penColor = QApplication::palette("QWidget").color(QPalette::Active,QPalette::ButtonText);


  painter->save();
  painter->setPen(QPen(penColor));
  painter->setFont(font);
  painter->drawText(rect, Qt::AlignCenter|Qt::AlignVCenter, mLetter);

  painter->restore();
}
QPixmap QFontIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
  QPixmap pix(size);
  pix.fill(Qt::transparent);

  QPainter painter(&pix);
  paint(&painter, QRect(QPoint(0,0),size), mode, state);
  return pix;

}
void QFontIconEngine::setFontFamily(const QString &family)
{
  mFontFamily = family;
}
void QFontIconEngine::setLetter(const QChar &letter)
{
  mLetter = letter;
}
void QFontIconEngine::setBaseColor(const QColor &baseColor)
{
  mBaseColor = baseColor;
}
QIconEngine *QFontIconEngine::clone() const
{
  QFontIconEngine * engine = new QFontIconEngine;
  engine->setFontFamily(mFontFamily);
  engine->setBaseColor(mBaseColor);
  return engine;
}
//=============================================================================
//=============================================================================
