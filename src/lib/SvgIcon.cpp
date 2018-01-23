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
#include "SvgIcon.h"
#include <QSvgRenderer>
#include <QPainter>
//=============================================================================
SvgIcon::SvgIcon(const QString &fileName, const QColor &color)
 : QIcon(renderSvgPixmap(fileName,color))
{
}
//=============================================================================
QByteArray SvgIcon::svgData(const QString &fileName,const QColor &color)
{
  // open svg resource load contents to qbytearray
  QFile file(fileName);
  file.open(QIODevice::ReadOnly);
  QByteArray baData = file.readAll();
  // load svg contents to xml document and edit contents
  QDomDocument doc;
  doc.setContent(baData);
  // recurivelly change color
  setAttrRecur(doc.documentElement(), "path", "fill", color.name());
  setAttrRecur(doc.documentElement(), "polygon", "fill", color.name());
  setAttrRecur(doc.documentElement(), "circle", "fill", color.name());
  setAttrRecur(doc.documentElement(), "rect", "fill", color.name());
  return doc.toByteArray();
}
//=============================================================================
QPixmap SvgIcon::renderSvgPixmap(const QString &fileName,const QColor &color) const
{
  // create svg renderer with edited contents
  QSvgRenderer svgRenderer(svgData(fileName,color));
  // create pixmap target (could be a QImage)
  QPixmap pix(svgRenderer.defaultSize());
  pix.fill(Qt::transparent);
  // create painter to act over pixmap
  QPainter pixPainter(&pix);
  // use renderer to render over painter which paints on pixmap
  svgRenderer.render(&pixPainter);
  return pix;
}
//=============================================================================
void SvgIcon::setAttrRecur(QDomElement elem, QString strtagname, QString strattr, QString strattrval)
{
  // if it has the tagname then overwritte desired attribute
  if (elem.tagName().compare(strtagname) == 0){
    elem.setAttribute(strattr, strattrval);
  }
  // loop all children
  for (int i = 0; i < elem.childNodes().count(); i++){
    if (!elem.childNodes().at(i).isElement()) continue;
    setAttrRecur(elem.childNodes().at(i).toElement(), strtagname, strattr, strattrval);
  }
}
//=============================================================================
