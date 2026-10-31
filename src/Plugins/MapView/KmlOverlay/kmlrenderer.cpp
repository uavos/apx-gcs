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
#include "kmlrenderer.h"

#include <cmath>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

namespace {

constexpr double kTileSize = 256.0;

// chart style: no fill, a translucent band inside the boundary and a thin solid outline
constexpr qreal kBandWidth = 12.0; // logical px
constexpr int kBandAlpha = 70;
constexpr qreal kOutlineWidth = 1.3; // logical px
constexpr qreal kDotRadius = 1.6;    // logical px, zones smaller than a few pixels
constexpr qreal kDotMaxSize = 2.5;   // logical px

// small labels at the zone centroid, only when zoomed in and the zone is big enough on screen
constexpr double kLabelMinZoom = 10.0;
constexpr qreal kLabelMinSize = 64.0; // logical px
constexpr qreal kLabelFontPx = 12.0;  // logical px
constexpr int kLabelAlpha = 235;

QPolygonF toPixels(const QVector<QPointF> &ring,
                   const QPointF &origin,
                   double scale,
                   double minStep)
{
    QPolygonF out;
    out.reserve(ring.size());
    QPointF last;
    for (const auto &p : ring) {
        const QPointF px((p.x() - origin.x()) * scale, (p.y() - origin.y()) * scale);
        // skip vertices that land on the same pixel as the previous one
        if (!out.isEmpty() && qAbs(px.x() - last.x()) < minStep
            && qAbs(px.y() - last.y()) < minStep)
            continue;
        out.append(px);
        last = px;
    }
    return out;
}

// returns the label rect, or a null rect when nothing was drawn
QRectF drawLabel(QPainter &painter,
                 const QFont &font,
                 const QFontMetricsF &fm,
                 const KmlZone &zone,
                 const QPointF &center,
                 const QColor &color,
                 qreal dpr,
                 const QVector<QRectF> &placed)
{
    QStringList lines;
    if (!zone.name.isEmpty())
        lines << zone.name;
    if (!zone.upper.isEmpty())
        lines << zone.upper;
    if (!zone.lower.isEmpty())
        lines << zone.lower;
    if (lines.isEmpty())
        return {};

    const qreal lh = fm.height();
    qreal w = 0;
    for (const auto &l : lines)
        w = qMax(w, fm.horizontalAdvance(l));
    const qreal h = lh * lines.size();
    const QRectF box(center.x() - w / 2, center.y() - h / 2, w, h);

    // skip labels that would overlap an already drawn one
    const QRectF guard = box.adjusted(-2 * dpr, -2 * dpr, 2 * dpr, 2 * dpr);
    for (const auto &r : placed) {
        if (r.intersects(guard))
            return {};
    }

    QPainterPath text;
    for (int i = 0; i < lines.size(); ++i) {
        const qreal lw = fm.horizontalAdvance(lines.at(i));
        text.addText(box.left() + (w - lw) / 2, box.top() + i * lh + fm.ascent(), font, lines.at(i));
    }

    // divider between upper and lower limits like on aeronautical charts
    if (!zone.upper.isEmpty() && !zone.lower.isEmpty()) {
        const int upperIndex = zone.name.isEmpty() ? 0 : 1;
        const qreal y = box.top() + (upperIndex + 1) * lh - 1.0 * dpr;
        text.addRect(QRectF(box.left(), y - 0.5 * dpr, w, qMax<qreal>(1.0, 1.0 * dpr)));
    }

    QColor textColor = color;
    textColor.setAlpha(kLabelAlpha);

    painter.setPen(Qt::NoPen);
    painter.setBrush(textColor);
    painter.drawPath(text);

    return guard;
}

} // namespace

double KmlRenderer::worldSize(double zoom)
{
    return kTileSize * std::pow(2.0, zoom);
}

KmlRenderResult KmlRenderer::render(const KmlRenderRequest &request)
{
    KmlRenderResult result;
    result.data = request.data;
    result.rect = request.rect;
    result.zoom = request.zoom;
    result.dpr = request.dpr;
    result.serial = request.serial;

    if (!request.data || request.rect.isEmpty() || request.dpr <= 0)
        return result;

    const qreal dpr = request.dpr;
    const double scale = worldSize(request.zoom) * dpr; // device px per mercator unit
    const QSize size(qMax(1, qCeil(request.rect.width() * scale)),
                     qMax(1, qCeil(request.rect.height() * scale)));

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return result;
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont font;
    font.setPixelSize(qMax(1, qRound(kLabelFontPx * dpr)));
    font.setWeight(QFont::DemiBold);
    const QFontMetricsF fm(font);
    QVector<QRectF> placedLabels;

    const qreal band = kBandWidth * dpr;
    const QRectF imageRect(QPointF(0, 0), QSizeF(size));
    const QRectF cullRect = imageRect.adjusted(-band, -band, band, band);
    const QPointF origin = request.rect.topLeft();
    const bool labels = request.zoom >= kLabelMinZoom;
    const double minStep = 0.6 * dpr;

    for (const KmlZone &zone : request.data->zones) {
        const QRectF bb((zone.bbox.left() - origin.x()) * scale,
                        (zone.bbox.top() - origin.y()) * scale,
                        zone.bbox.width() * scale,
                        zone.bbox.height() * scale);
        if (!bb.intersects(cullRect))
            continue;

        QColor color = zone.color;
        color.setAlpha(255);
        QColor bandColor = color;
        bandColor.setAlpha(kBandAlpha);

        if (qMax(bb.width(), bb.height()) < kDotMaxSize * dpr) {
            QColor dot = color;
            dot.setAlpha(200);
            painter.setPen(Qt::NoPen);
            painter.setBrush(dot);
            painter.drawEllipse(bb.center(), kDotRadius * dpr, kDotRadius * dpr);
            continue;
        }

        QPainterPath path;
        path.setFillRule(Qt::OddEvenFill);
        path.addPolygon(toPixels(zone.outer, origin, scale, minStep));
        path.closeSubpath();
        for (const auto &hole : zone.holes) {
            const QPolygonF hp = toPixels(hole, origin, scale, minStep);
            if (hp.size() < 3)
                continue;
            path.addPolygon(hp);
            path.closeSubpath();
        }

        const qreal side = qMin(bb.width(), bb.height());
        if (side <= 2 * band) {
            // the band would cover the whole zone anyway
            painter.setPen(Qt::NoPen);
            painter.setBrush(bandColor);
            painter.drawPath(path);
        } else {
            // stroke a double width band and clip the outer half away
            painter.save();
            painter.setClipPath(path);
            painter.setPen(QPen(bandColor, band * 2, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
            painter.restore();
        }

        painter.setPen(QPen(color, kOutlineWidth * dpr));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        if (labels && side >= kLabelMinSize * dpr) {
            const QPointF c((zone.labelPos.x() - origin.x()) * scale,
                            (zone.labelPos.y() - origin.y()) * scale);
            const QRectF placed = drawLabel(painter, font, fm, zone, c, color, dpr, placedLabels);
            if (!placed.isNull())
                placedLabels.append(placed);
        }
    }

    painter.end();
    result.image = image;
    return result;
}
