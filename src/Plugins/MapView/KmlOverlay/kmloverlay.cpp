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

#include "kmlparser.h"
#include "kmlrasteritem.h"
#include <App/App.h>
#include <QFileDialog>
#include <QGeoRectangle>
#include <QMessageBox>

#include "quazip/quazip.h"
#include "quazip/quazipfile.h"

namespace {

constexpr qint64 kMaxKmlSize = 128 * 1024 * 1024;

bool isKmzArchive(const QString &path, QFile &file)
{
    if (path.endsWith(QStringLiteral(".kmz"), Qt::CaseInsensitive))
        return true;

    // KMZ is a regular zip archive, check the local file header signature
    const QByteArray magic = file.peek(4);
    return magic == QByteArrayLiteral("PK\x03\x04");
}

QByteArray readKmzData(const QString &path, QString &error)
{
    QuaZip zip(path);
    if (!zip.open(QuaZip::mdUnzip)) {
        error = QStringLiteral("Unable to open KMZ archive");
        return {};
    }

    // KMZ keeps the main document as doc.kml in the archive root,
    // fall back to the first *.kml entry if it is named differently
    QString entry;
    for (const QString &name : zip.getFileNameList()) {
        if (!name.endsWith(QStringLiteral(".kml"), Qt::CaseInsensitive))
            continue;
        if (name.compare(QStringLiteral("doc.kml"), Qt::CaseInsensitive) == 0) {
            entry = name;
            break;
        }
        if (entry.isEmpty() || !name.contains('/'))
            entry = name;
    }

    if (entry.isEmpty()) {
        error = QStringLiteral("No KML document found in KMZ archive");
        return {};
    }

    if (!zip.setCurrentFile(entry)) {
        error = QStringLiteral("Unable to locate %1 in KMZ archive").arg(entry);
        return {};
    }

    QuaZipFileInfo64 info;
    if (zip.getCurrentFileInfo(&info) && info.uncompressedSize > quint64(kMaxKmlSize)) {
        error = QStringLiteral("KML document in KMZ archive is too large");
        return {};
    }

    QuaZipFile kml(&zip);
    if (!kml.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Unable to read %1 from KMZ archive").arg(entry);
        return {};
    }

    const QByteArray data = kml.readAll();
    kml.close();

    if (data.size() > kMaxKmlSize) {
        error = QStringLiteral("KML document in KMZ archive is too large");
        return {};
    }

    return data;
}

QByteArray readKmlData(const QString &path, QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Unable to open file");
        return {};
    }

    if (isKmzArchive(path, file)) {
        file.close();
        return readKmzData(path, error);
    }

    if (file.size() > kMaxKmlSize) {
        error = QStringLiteral("KML file is too large");
        return {};
    }

    return file.readAll();
}

} // namespace

KmlOverlay::KmlOverlay(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("KML Overlay"),
           tr("KML objects overlay"),
           Group)
{
    KmlRasterItem::registerQmlType();

    setIcon("google-earth");

    f_open = new Fact(this, "open", tr("Open..."), tr("Open KML or KMZ file"));
    f_open->setIcon("import");

    f_visible = new Fact(this, "show", tr("Show overlay"), "", Fact::Bool);
    f_visible->setIcon("check");
    f_visible->setValue(true);

    f_opacity = new Fact(this, "opacity", tr("Opacity"), "", Fact::Float);
    f_opacity->setMin(0);
    f_opacity->setMax(1);
    f_opacity->setValue(1);

    connect(f_open, &Fact::triggered, this, &KmlOverlay::onOpenTriggered);

    connect(&m_importWatcher, &QFutureWatcherBase::finished,
            this, &KmlOverlay::onImportFinished);

    // the import thread reports parse progress in percent, the UI polls it
    m_progressTimer.setInterval(100);
    connect(&m_progressTimer, &QTimer::timeout, this, [this]() {
        if (!m_parseProgress)
            return;
        const int parsed = m_parseProgress->load();
        // parsing is 1..95, the rest is building the dataset
        const int progress = qBound(1, 1 + parsed * 94 / 100, 95);
        setProgress(progress);
        f_open->setProgress(progress);
        setValue(tr("Parsing KML: %1%").arg(parsed));
    });

    loadQml("qrc:/" PLUGIN_NAME "/KmlOverlayPlugin.qml");
}

QGeoCoordinate KmlOverlay::getCenter() const
{
    return m_center;
}

int KmlOverlay::zonesCount() const
{
    return m_data ? int(m_data->zones.size()) : 0;
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
    }

    emit loadingChanged();
}

void KmlOverlay::onOpenTriggered()
{
    if (m_loading)
        return;

    QString path = QFileDialog::getOpenFileName(nullptr,
                                                tr("Open KML"),
                                                QDir::homePath(),
                                                tr("KML files (*.kml *.kmz);;KML (*.kml);;KMZ (*.kmz)"));
    if (path.isEmpty())
        return;

    setLoading(true);

    m_parseProgress = QSharedPointer<std::atomic<int>>::create(0);
    setProgress(1);
    f_open->setProgress(1);
    setValue(tr("Reading KML..."));
    m_progressTimer.start();

    auto parseProgress = m_parseProgress;
    m_importWatcher.setFuture(QtConcurrent::run([path, parseProgress]() -> ImportResult {
        ImportResult result;

        const QByteArray data = readKmlData(path, result.error);
        if (!result.error.isEmpty())
            return result;

        if (data.isEmpty()) {
            result.error = QStringLiteral("KML file is empty");
            return result;
        }

        KmlParser parser;
        parser.parse(data, [parseProgress](int percent) { parseProgress->store(percent); });

        result.data = KmlDataSet::fromPolygons(parser.getPolygons());

        if (result.data->zones.isEmpty())
            result.error = QStringLiteral("No valid polygons found");

        return result;
    }));
}

void KmlOverlay::onImportFinished()
{
    m_progressTimer.stop();
    m_parseProgress.reset();

    ImportResult result = m_importWatcher.result();

    if (!result.error.isEmpty()) {
        setValue(tr("KML import failed"));
        setLoading(false);
        QMessageBox::warning(nullptr, tr("KML Overlay"), result.error);
        return;
    }

    m_data = result.data;
    m_center = KmlMercator::toGeo(m_data->bbox.center());

    setValue(tr("%1 KML zones loaded").arg(m_data->zones.size()));

    emit dataChanged();
    if (m_center.isValid())
        emit centerChanged();

    setLoading(false);
}
