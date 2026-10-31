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
#pragma once

#include "kmldataset.h"
#include <QImage>

struct KmlRenderRequest
{
    KmlDataSetPtr data;
    QRectF rect;    // mercator unit rect covered by the image
    double zoom{0}; // map zoom level the image is rendered for
    qreal dpr{1};
    quint64 serial{0};
};

struct KmlRenderResult
{
    KmlDataSetPtr data;
    QImage image;
    QRectF rect;
    double zoom{0};
    qreal dpr{1};
    quint64 serial{0};
};

namespace KmlRenderer {

// logical pixels per mercator unit at the given zoom level
double worldSize(double zoom);

// draws all zones of the request into a transparent image, safe to call from a worker thread
KmlRenderResult render(const KmlRenderRequest &request);

} // namespace KmlRenderer
