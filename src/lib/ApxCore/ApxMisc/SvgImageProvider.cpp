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
#include "SvgImageProvider.h"
#include <App/AppLog.h>
#include <QDebug>
#include <QPainter>
#include <QUrl>

SvgImageProvider::SvgImageProvider(const QString &basePath)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_basePath(basePath)
{}

SvgImageProvider::~SvgImageProvider()
{
    //qDeleteAll(m_renderers.values());
}

QSvgRenderer *SvgImageProvider::loadRenderer(const QString &svgFile)
{
    if (svgFile.isNull())
        return nullptr;
    QString s = svgFile.trimmed();
    while (s.contains("//"))
        s.replace("//", "/");
    if (s.size() < 5)
        return nullptr;

    QSvgRenderer *renderer = m_renderers.value(s);

    if (!renderer) {
        //qDebug()<<svgFile;
        renderer = new QSvgRenderer(this);
        connect(renderer, &QSvgRenderer::destroyed, this, [this, renderer]() {
            m_renderers.remove(m_renderers.key(renderer));
        });

        // convert path to be relative to base
        QString fn;
        if (m_basePath.contains(':') && s.contains(':'))
            s.remove(0, s.indexOf('/') + 1);
        fn = m_basePath + "/"
             + s; //QUrl::fromLocalFile(m_basePath).resolved(svgFile).toLocalFile();

        if (!renderer->isValid()) {
            renderer->load(fn);
        }

        if (!renderer->isValid()) {
            apxConsoleW() << "Failed to load svg file:" << svgFile << fn;
            delete renderer;
            return nullptr;
        }

        m_renderers.insert(s, renderer);
    }

    return renderer;
}

/**
   Supported id format: fileName[!elementName[?parameters]]
   where parameters may be:
   vslice=1:2;hslice=2:4 - use the 3rd horizontal slice of total 4 slices, slice numbering starts from 0
   borders=1 - 1 pixel wide transparent border

   requestedSize is related to the whole element size, even if slice is requested.

   usage:

   Image {
       source: "image://svg/pfd.svg!world"
   }
 */
QImage SvgImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    //if(size->isNull())return QImage(1,1,QImage::Format_Mono);
    //if(requestedSize.isNull())return QImage(1,1,QImage::Format_Mono);

    QString svgFile = id;
    QStringList params;

    int sepPos = id.indexOf('?');
    if (sepPos != -1) {
        svgFile = id.left(sepPos);
        params = id.mid(sepPos + 1).split('&');
    }

    QString element;
    int hSlicesCount = 0;
    int hSlice = 0;
    int vSlicesCount = 0;
    int vSlice = 0;
    int border = 0;
    QString color;
    for (int i = 0; i < params.size(); ++i) {
        const QString s = params.at(i);
        int idel = s.indexOf('=');
        if (idel <= 0)
            continue;
        const QString &sn = s.left(idel);
        const QString &sv = s.mid(idel + 1);
        if (sn == "e") {
            element = sv;
        } else if (sn == "hslice") {
            static QRegularExpression rx("(\\d+):(\\d+)");
            QRegularExpressionMatch rxm = rx.match(sv);
            if (rxm.hasMatch()) {
                hSlice = rxm.captured(1).toInt();
                hSlicesCount = rxm.captured(2).toInt();
            }
        } else if (sn == "vslice") {
            static QRegularExpression rx("(\\d+):(\\d+)");
            auto rxm = rx.match(sv);
            if (rxm.hasMatch()) {
                vSlice = rxm.captured(1).toInt();
                vSlicesCount = rxm.captured(2).toInt();
            }
        } else if (sn == "border") {
            static QRegularExpression rx("(\\d+)");
            auto rxm = rx.match(sv);
            if (rxm.hasMatch()) {
                border = rxm.captured(1).toInt();
            }
        } else if (sn == "color") {
            color = sv;
        }
    }

    if (size) {
        *size = QSize();
    }

    QSvgRenderer *renderer = loadRenderer(svgFile);
    if (!renderer) {
        return QImage(1, 1, QImage::Format_Mono);
    }
    if (element.isEmpty()) {
        renderer->deleteLater(); //don't cache
    }

    qreal xScale = 1.0;
    qreal yScale = 1.0;

    QSize docSize = renderer->defaultSize();

    if (!requestedSize.isEmpty()) {
        if (!element.isEmpty()) {
            QSize rsz(requestedSize);
            double sz; //=1024;
            /*if(rsz.width()>sz||rsz.height()>sz)
          rsz.scale(rsz/2,Qt::KeepAspectRatio);*/
            sz = 2048;
            if (rsz.width() > sz || rsz.height() > sz)
                rsz.scale(QSizeF(sz, sz).toSize(), Qt::KeepAspectRatio);
            QRectF elementBounds = renderer->boundsOnElement(element);
            xScale = qreal(rsz.width()) / elementBounds.width();
            yScale = qreal(rsz.height()) / elementBounds.height();
        } /*else if (!docSize.isEmpty()) {
          xScale = qreal(requestedSize.width()) / docSize.width();
          yScale = qreal(requestedSize.height()) / docSize.height();
      }*/
    }

    // keep the aspect ratio
    xScale = yScale = qMin(xScale, yScale);

    if (!element.isEmpty()) {
        if (!renderer->elementExists(element)) {
            apxConsoleW() << "invalid element:" << element << "of" << svgFile;
            return QImage();
        }

        QRectF elementBounds = renderer->boundsOnElement(element);
        int elementWidth = qRound(elementBounds.width() * xScale);
        int elementHeigh = qRound(elementBounds.height() * yScale);
        int w = elementWidth;
        int h = elementHeigh;
        int x = 0;
        int y = 0;
        if (elementWidth <= 0 || elementHeigh <= 0)
            return QImage(1, 1, QImage::Format_ARGB32_Premultiplied);

        if (hSlicesCount > 1) {
            x = (w * hSlice) / hSlicesCount;
            w = (w * (hSlice + 1)) / hSlicesCount - x;
        }

        if (vSlicesCount > 1) {
            y = (h * (vSlice)) / vSlicesCount;
            h = (h * (vSlice + 1)) / vSlicesCount - y;
        }

        QImage img(w + border * 2, h + border * 2, QImage::Format_ARGB32_Premultiplied);
        if (!img.isNull()) {
            img.fill(0);
            QPainter p(&img);
            if (p.isActive()) {
                p.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing
                                 | QPainter::SmoothPixmapTransform);

                p.translate(-x + border, -y + border);
                QRectF rElement(0, 0, elementBounds.width(), elementBounds.height());
                p.scale(w / rElement.width(), h / rElement.height());
                renderer->render(&p, element, rElement);
            }
        }
        if (size) {
            *size = QSize(w, h);
        }
        //if(img.size().isNull()) return QImage(1,1,QImage::Format_ARGB32_Premultiplied);

        //img.save("/tmp/img/" + element + params.join('.') + ".png");
        //qDebug() << img.size();
        return img;
    }

    // render the whole svg file
    int w = qRound(docSize.width() * xScale);
    int h = qRound(docSize.height() * yScale);

    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    if (!img.isNull()) {
        img.fill(0);
        QPainter p(&img);
        if (p.isActive()) {
            p.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing
                             | QPainter::SmoothPixmapTransform);

            p.scale(xScale, yScale);
            renderer->render(&p, QRectF(QPointF(), QSizeF(docSize)));
        }
    }
    if (size) {
        *size = QSize(w, h);
    }
    return img;
}

QPixmap SvgImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    return QPixmap::fromImage(requestImage(id, size, requestedSize));
}

/*!
   \fn SvgImageProvider::scaledElementBounds(const QString &svgFile, const QString &element)

   Returns the bound of \a element in logical coordinates,
   scalled to the default size of svg document (so the bounds of whole doc would be (0,0,1,1) ).
 */
QRectF SvgImageProvider::elementBounds(const QString &svgFile, const QString &elementName)
{
    if (svgFile.isNull())
        return QRectF();
    QSvgRenderer *renderer = loadRenderer(svgFile);

    if (!renderer) {
        return QRectF();
    }

    if (!renderer->elementExists(elementName)) {
        //apxConsoleW() << "invalid element:" << elementName << "of" << svgFile;
        return QRectF();
    }

    QRectF elementBounds = renderer->boundsOnElement(elementName);
    QTransform matrix = renderer->transformForElement(elementName);
    elementBounds = matrix.mapRect(elementBounds);

    return elementBounds;
    /*QSize docSize  = renderer->defaultSize();
    return QRectF(elementBounds.x() / docSize.width(),
                  elementBounds.y() / docSize.height(),
                  elementBounds.width() / docSize.width(),
                  elementBounds.height() / docSize.height());*/
}
