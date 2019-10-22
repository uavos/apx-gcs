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
#ifndef SvgMaterialIcon_H
#define SvgMaterialIcon_H
//=============================================================================
#include <QIcon>
#include <QIconEngine>
#include <QQmlEngine>
#include <QtCore>
//=============================================================================
class MaterialIcon : public QIcon
{
public:
    MaterialIcon(const QString &name, const QColor &color = QColor(Qt::white));

    static QChar getChar(const QString &name);

private:
    static QHash<QString, QChar> map;
    static void updateMap();

    QIcon icon(const QString &name, const QColor &color) const;
};
//=============================================================================
class QFontIconEngine : public QIconEngine
{
public:
    QFontIconEngine();
    virtual ~QFontIconEngine();
    virtual void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
        Q_DECL_OVERRIDE;
    virtual QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
    void setFontFamily(const QString &family);
    // define icon code using QChar or implicit using ushort ...
    void setLetter(const QChar &letter);
    // You can set a base color. I don't advice. Keep system color
    void setBaseColor(const QColor &baseColor);
    virtual QIconEngine *clone() const override;

private:
    QString mFontFamily;
    QChar mLetter;
    QColor mBaseColor;
};
//=============================================================================
#endif
