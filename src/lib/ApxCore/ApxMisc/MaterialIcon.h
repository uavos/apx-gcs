/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
