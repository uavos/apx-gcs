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
#ifndef SVGIMAGEPROVIDER_H_
#define SVGIMAGEPROVIDER_H_

#include <QMap>
#include <QObject>
#include <QQuickImageProvider>
#include <QSvgRenderer>

class SvgImageProvider : public QObject, public QQuickImageProvider
{
    Q_OBJECT
public:
    SvgImageProvider(const QString &basePath);
    ~SvgImageProvider();

    QSvgRenderer *loadRenderer(const QString &svgFile);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);

    Q_INVOKABLE QRectF elementBounds(const QString &svgFile, const QString &elementName);

private:
    QMap<QString, QSvgRenderer *> m_renderers;
    QString m_basePath;
    QMap<QString, QPointF> m_center;
};

#endif // ifndef SVGIMAGEPROVIDER_H_
