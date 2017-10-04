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
#ifndef ItemText_H
#define ItemText_H
#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsItem>
#include "ItemBase.h"
//=============================================================================
class ItemText : public QObject, public QGraphicsItem
{
  Q_OBJECT
  Q_INTERFACES(QGraphicsItem)
public:
  ItemText(QString text=QString(),QColor color=QColor(),QColor fontColor=QColor(),Qt::Alignment alignment=Qt::AlignVCenter|Qt::AlignLeft);
  enum {Type=ItemBase::TypeItemText};
  int type() const{return Type;}
  QRectF boundingRect() const;

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);
  void mousePressEvent(QGraphicsSceneMouseEvent * event);
private:
  double margin(void) const;
  QColor m_color,m_fontColor;
  QFont m_font;
  Qt::Alignment m_alignment;
  QString text;
  QPixmap pixmap;
  void realign();
  void updatePixmap();

public slots:
  void setText(QString text);
  void setFont(QFont v);
  void setColor(QColor v);
  void setFontColor(QColor v);
  void setAlignment(Qt::Alignment v);
signals:
  void clicked();
};
//=============================================================================
#endif
