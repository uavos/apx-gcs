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
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include "ItemText.h"
#include "QMandala.h"
//=============================================================================
ItemText::ItemText(QString text,QColor color,QColor fontColor, Qt::Alignment alignment)
  : QObject(),QGraphicsItem(),m_color(color),m_fontColor(fontColor),m_alignment(alignment)
{
  setFlags(ItemIgnoresTransformations);
  setCacheMode(QGraphicsItem::ItemCoordinateCache);
  m_font.setPixelSize(12);
  setText(text);
  realign();
}
//=============================================================================
void ItemText::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)
  painter->setRenderHint(QPainter::SmoothPixmapTransform,false);
  painter->setClipRect(option->exposedRect);
  painter->drawPixmap(0,0,pixmap);
}
//=============================================================================
void ItemText::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  QGraphicsItem::mousePressEvent(event);
  if(event->button()==Qt::LeftButton)emit clicked();
}
//=============================================================================
void ItemText::updatePixmap()
{
  if(text.isEmpty()){
    pixmap=QPixmap();
    return;
  }
  QPainterPath path;
  const QStringList &st=text.split('\n');
  qreal y=0;
  foreach(const QString &s,st){
    path.addText(1,y,m_font,s);
    y=path.boundingRect().height()+2;
  }
  double m=margin(),m2=m*2.0;
  path.translate(m-1,-(int)(path.boundingRect().top()-m-1));
  QRectF r=path.boundingRect().adjusted(-1,0,1,1);
  pixmap=QPixmap((r.size()+QSizeF(m2,m2)).toSize());
  QPainter p(&pixmap);
  p.setRenderHint(QPainter::Antialiasing);
  if(m_color.isValid()){
    p.fillRect(pixmap.rect(),QBrush(m_color));
    p.setPen(Qt::NoPen);
    p.setBrush(m_fontColor.isValid()?m_fontColor:Qt::black);
    p.drawPath(path);
  }else{
    p.setPen(QPen(Qt::black,4));
    p.setBrush(Qt::black);
    p.drawPath(path);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawPath(path);
  }
}
//=============================================================================
double ItemText::margin(void) const
{
  if(!m_color.isValid())return 4;
  if(m_font.pixelSize()>=20)return m_font.pixelSize()/5;
  return 1;
}
//=============================================================================
void ItemText::setText(QString text)
{
  if(this->text==text)return;
  prepareGeometryChange();
  this->text=text;
  updatePixmap();
  realign();
  update();
  //setVisible(!text.isEmpty());
}
//=============================================================================
QRectF ItemText::boundingRect() const
{
  return QRectF(pixmap.rect());
}
//=============================================================================
void ItemText::setFont(QFont v)
{
  m_font=v;
  updatePixmap();
  update();
}
//=============================================================================
void ItemText::setColor(QColor v)
{
  if(m_color==v)return;
  m_color=v;
  updatePixmap();
  realign();
}
void ItemText::setFontColor(QColor v)
{
  if(m_fontColor==v)return;
  m_fontColor=v;
  updatePixmap();
  realign();
}
//=============================================================================
void ItemText::setAlignment(Qt::Alignment v)
{
  if(m_alignment==v)return;
  m_alignment=v;
  realign();
}
//=============================================================================
void ItemText::realign()
{
  if(!(parentItem()&&m_alignment))return;
  QRectF r=parentItem()->boundingRect();
  QRectF b=boundingRect();
  QPointF p(pos());
  if(m_alignment&Qt::AlignHCenter)
    p.setX(-(b.width())/2);
  if(m_alignment&Qt::AlignLeft)
    p.setX(r.width()+r.left());
  if(m_alignment&Qt::AlignRight)
    p.setX(-b.width()-r.left());
  if(m_alignment&Qt::AlignVCenter)
    p.setY(-(b.height())/2);
  if(m_alignment&Qt::AlignTop)
    p.setY(r.bottom());
  if(m_alignment&Qt::AlignBottom)
    p.setY(-b.height());
  if(pos()!=p)setPos(p);
  else update();
}
//=============================================================================

