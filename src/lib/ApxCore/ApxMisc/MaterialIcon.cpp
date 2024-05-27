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
#include "MaterialIcon.h"
#include <App/AppLog.h>
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QPalette>
#include <QSvgRenderer>

QHash<QString, QString> MaterialIcon::map;

MaterialIcon::MaterialIcon(const QString &name, const QColor &color)
    : QIcon(icon(name, color))
{
    //https://materialdesignicons.com/
}

QIcon MaterialIcon::icon(const QString &name, const QColor &color) const
{
    QString c = getChar(name);
    if (c.isNull()) {
        return QIcon();
    }
    QFontIconEngine *engine = new QFontIconEngine;
    engine->setFontFamily("Material Design Icons");
    engine->setLetter(c);
    engine->setBaseColor(color);
    return QIcon(engine);
}

QString MaterialIcon::getChar(const QString &name)
{
    if (map.isEmpty())
        updateMap();

    if (name.isEmpty())
        return QChar();
    QString c = map.value(name);
    if (c.isEmpty()) {
        apxConsoleW() << "Material icon is missing:" << name;
    }
    return c;
}

void MaterialIcon::updateMap()
{
    QFile res;
    res.setFileName(":/icons/material-icons.json");
    if (res.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(res.readAll());
        res.close();
        QJsonObject obj = json.object();
        for (auto v = obj.constBegin(); v != obj.constEnd(); ++v) {
            QString s = v.value().toVariant().toString(); //.toString();
            QString c = 0;
            if (s.size() == 1) {
                c = s.at(0);
            } else if (s.startsWith("\\u")) {
                std::wstring str = s.toStdWString();
                s = QString::fromStdWString(str);
                if (s.size() == 1)
                    c = s.at(0);
            } else if (s.startsWith("\\", Qt::CaseInsensitive)) {
                const char32_t code = s.mid(1).toUInt(nullptr, 16);
                c = QString::fromUcs4(&code, 1);
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

QFontIconEngine::QFontIconEngine()
    : QIconEngine()
{}
void QFontIconEngine::paint(QPainter *painter,
                            const QRect &rect,
                            QIcon::Mode mode,
                            QIcon::State state)
{
    Q_UNUSED(state)

    QFont font = QFont(mFontFamily);
    int drawSize = rect.height(); //qRound(rect.height() * 0.8);
    font.setPixelSize(drawSize < 5 ? 5 : drawSize);

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
void QFontIconEngine::setLetter(const QString &letter)
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
