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
#include "MaterialIcon.h"
#include <App/AppLog.h>
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QPalette>
#include <QSvgRenderer>
//=============================================================================
QHash<QString, QChar> MaterialIcon::map;

MaterialIcon::MaterialIcon(const QString &name, const QColor &color)
    : QIcon(icon(name, color))
{
    //https://materialdesignicons.com/
}
//=============================================================================
QIcon MaterialIcon::icon(const QString &name, const QColor &color) const
{
    QChar c = getChar(name);
    if (c.isNull()) {
        return QIcon();
    }
    QFontIconEngine *engine = new QFontIconEngine;
    engine->setFontFamily("Material Design Icons");
    engine->setLetter(c);
    engine->setBaseColor(color);
    return QIcon(engine);
}
//=============================================================================
QChar MaterialIcon::getChar(const QString &name)
{
    if (map.isEmpty())
        updateMap();

    if (name.isEmpty())
        return QChar();
    QChar c = map.value(name);
    if (c == '\0') {
        apxConsoleW() << "Material icon is missing:" << name;
    }
    return c;
}
//=============================================================================
void MaterialIcon::updateMap()
{
    QFile res;
    res.setFileName(":/icons/material-icons.ttf");
    if (res.open(QIODevice::ReadOnly)) {
        QFontDatabase::addApplicationFontFromData(res.readAll());
        res.close();
    }
    res.setFileName(":/icons/material-icons.json");
    if (res.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(res.readAll());
        res.close();
        QJsonObject obj = json.object();
        for (auto v = obj.constBegin(); v != obj.constEnd(); ++v) {
            QString s = v.value().toVariant().toString(); //.toString();
            QChar c = 0;
            if (s.size() == 1) {
                c = s.at(0);
            } else if (s.startsWith("\\u")) {
                std::wstring str = s.toStdWString();
                s = QString::fromStdWString(str);
                if (s.size() == 1)
                    c = s.at(0);
            } else if (s.startsWith("\\F", Qt::CaseInsensitive)) {
                /*QTextCodec *codec = QTextCodec::codecForName("UTF-32BE");
                QString sc = codec->toUnicode(s.mid(1).prepend("\\U").toUtf8());
                uint v = s.mid(1).prepend("0x").toUInt(nullptr, 16);
                c = QChar(v); //sc.at(0);*/
                /*bool ok;
                uint v = s.mid(1).toUInt(&ok, 16);
                if (ok)
                    c = v;*/

                //qDebug() << s << sc;
            }
            if (c == '\0') {
                //qWarning() << v.key() << s << v.value() << s.size();
                continue;
            }
            map[v.key()] = c;
        }
    }
    if (map.isEmpty())
        map.insert("", QChar('\0'));
}
//=============================================================================
//=============================================================================
QFontIconEngine::QFontIconEngine()
    : QIconEngine()
{}
QFontIconEngine::~QFontIconEngine() {}
void QFontIconEngine::paint(QPainter *painter,
                            const QRect &rect,
                            QIcon::Mode mode,
                            QIcon::State state)
{
    Q_UNUSED(state)

    QFont font = QFont(mFontFamily);
    int drawSize = rect.height(); //qRound(rect.height() * 0.8);
    font.setPixelSize(drawSize > 1 ? drawSize : 1);

    QColor penColor;
    if (!mBaseColor.isValid())
        penColor = QApplication::palette("QWidget").color(QPalette::Normal, QPalette::ButtonText);
    else
        penColor = mBaseColor;

    if (mode == QIcon::Disabled)
        penColor = QApplication::palette("QWidget").color(QPalette::Disabled, QPalette::ButtonText);

    if (mode == QIcon::Selected)
        penColor = QApplication::palette("QWidget").color(QPalette::Active, QPalette::ButtonText);

    painter->save();
    painter->setPen(QPen(penColor));
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter | Qt::AlignVCenter, mLetter);

    painter->restore();
}
QPixmap QFontIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pix(size);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    paint(&painter, QRect(QPoint(0, 0), size), mode, state);
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
    QFontIconEngine *engine = new QFontIconEngine;
    engine->setFontFamily(mFontFamily);
    engine->setBaseColor(mBaseColor);
    return engine;
}
//=============================================================================
//=============================================================================
