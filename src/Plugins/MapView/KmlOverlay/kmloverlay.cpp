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
#include "kmloverlay.h"

#include "kmlgeopolygon.h"
#include "kmlparser.h"
#include <App/App.h>
#include <QFileDialog>
#include <QGeoRectangle>
#include <QMessageBox>

KmlOverlay::KmlOverlay(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("KML Overlay"),
           tr("KML objects overlay"),
           Group)
    , m_kmlPolygons(new KmlPolygonsModel())
{
    KmlGeoPolygon::registerQmlType();

    setIcon("google-earth");

    f_open = new Fact(this, "open", tr("Open..."), tr("Open KML file"));
    f_open->setIcon("import");

    f_visible = new Fact(this, "show", tr("Show overlay"), "", Fact::Bool);
    f_visible->setIcon("check");
    f_visible->setValue(true);

    f_opacity = new Fact(this, "opacity", tr("Opacity"), "", Fact::Float);
    f_opacity->setMin(0);
    f_opacity->setMax(1);
    f_opacity->setValue(1);

    connect(f_open, &Fact::triggered, this, &KmlOverlay::onOpenTriggered);
    connect(f_visible, &Fact::valueChanged, this, &KmlOverlay::onOverlayVisibleValueChanged);

    connect(&m_importWatcher, &QFutureWatcherBase::finished,
            this, &KmlOverlay::onImportFinished);

    m_importTimer.setInterval(0);
    connect(&m_importTimer, &QTimer::timeout, this, &KmlOverlay::importBatch);

    m_progressTimer.setInterval(120);
    connect(&m_progressTimer, &QTimer::timeout, this, [this]() {
        m_pulse = (m_pulse + 1) % 20;
        setProgress(m_pulse + 1);
        f_open->setProgress(m_pulse + 1);
    });

    loadQml("qrc:/" PLUGIN_NAME "/KmlOverlayPlugin.qml");
}

KmlPolygonsModel *KmlOverlay::getKmlPolygons() const
{
    return m_kmlPolygons;
}

QGeoCoordinate KmlOverlay::getCenter() const
{
    return m_center;
}

void KmlOverlay::updateKmlModels(const QGeoShape &shape)
{
    if (!shape.isValid())
        return;

    QRectF bb(gc2p(shape.boundingGeoRectangle().topLeft()),
              gc2p(shape.boundingGeoRectangle().bottomRight()));
    m_kmlPolygons->setBoundingBox(bb);
}

QPointF KmlOverlay::gc2p(const QGeoCoordinate &c)
{
    return QPointF(c.latitude(), c.longitude());
}

QGeoCoordinate KmlOverlay::p2gc(const QPointF &p)
{
    return QGeoCoordinate(p.x(), p.y());
}

void KmlOverlay::setLoading(bool loading)
{
    if (m_loading == loading)
        return;

    m_loading = loading;
    f_open->setEnabled(!loading);

    if (!loading) {
        m_progressTimer.stop();
        setProgress(-1);
        f_open->setProgress(-1);
        setLoadProgress(0);
    }

    emit loadingChanged();
}

void KmlOverlay::setLoadProgress(int progress)
{
    progress = qBound(0, progress, 100);
    if (m_loadProgress == progress)
        return;

    m_loadProgress = progress;
    emit loadProgressChanged();
}

void KmlOverlay::onOpenTriggered()
{
    if (m_loading)
        return;

    QString path = QFileDialog::getOpenFileName(nullptr,
                                                tr("Open kml"),
                                                QDir::homePath(),
                                                "KML (*.kml)");
    if (path.isEmpty())
        return;

    setLoading(true);
    setLoadProgress(0);

    m_pulse = 0;
    setProgress(1);
    f_open->setProgress(1);
    setValue(tr("Reading KML..."));
    m_progressTimer.start();

    m_importWatcher.setFuture(QtConcurrent::run([path]() -> ImportResult {
        ImportResult result;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            result.error = QStringLiteral("Unable to open file");
            return result;
        }

        if (file.size() > 128 * 1024 * 1024) {
            result.error = QStringLiteral("KML file is too large");
            return result;
        }

        const QByteArray data = file.readAll();
        file.close();

        if (data.isEmpty()) {
            result.error = QStringLiteral("KML file is empty");
            return result;
        }

        KmlParser parser;
        parser.parse(data);
        result.polygons = parser.getPolygons();

        if (result.polygons.isEmpty())
            result.error = QStringLiteral("No valid polygons found");

        return result;
    }));
}

void KmlOverlay::onImportFinished()
{
    m_progressTimer.stop();

    ImportResult result = m_importWatcher.result();

    if (!result.error.isEmpty()) {
        setValue(tr("KML import failed"));
        setLoading(false);
        QMessageBox::warning(nullptr, tr("KML Overlay"), result.error);
        return;
    }

    m_pendingPolygons = std::move(result.polygons);
    m_importIndex = 0;

    m_kmlPolygons->clearPolygons();

    setValue(tr("Adding KML zones..."));
    setLoadProgress(0);
    setProgress(20);
    f_open->setProgress(20);

    m_importTimer.start();
}

void KmlOverlay::importBatch()
{
    if (m_importIndex >= m_pendingPolygons.size()) {
        finishImport();
        return;
    }

    QElapsedTimer elapsed;
    elapsed.start();

    const int total = m_pendingPolygons.size();

    while (m_importIndex < total && elapsed.elapsed() < 6) {
        const int count = qMin(8, total - m_importIndex);
        m_kmlPolygons->appendPolygons(m_pendingPolygons, m_importIndex, count);
        m_importIndex += count;
    }

    const int progress = 20 + (m_importIndex * 79 / total);
    setLoadProgress(m_importIndex * 100 / total);
    setProgress(progress);
    f_open->setProgress(progress);

    setValue(tr("Adding zones: %1/%2").arg(m_importIndex).arg(total));
}

void KmlOverlay::finishImport()
{
    m_importTimer.stop();

    const QPointF center = m_kmlPolygons->center();
    m_center = p2gc(center);

    m_loadedPolygons = std::move(m_pendingPolygons);
    m_pendingPolygons.clear();
    m_importIndex = 0;

    setLoadProgress(100);
    setProgress(100);
    f_open->setProgress(100);

    setValue(tr("%1 KML zones loaded").arg(m_loadedPolygons.size()));

    if (m_center.isValid())
        emit centerChanged();

    if (!f_visible->value().toBool())
        m_kmlPolygons->clearPolygons();

    setLoading(false);
}

void KmlOverlay::onOverlayVisibleValueChanged()
{
    if (m_loading)
        return;

    if (f_visible->value().toBool())
        m_kmlPolygons->setPolygons(m_loadedPolygons);
    else
        m_kmlPolygons->clearPolygons();
}
