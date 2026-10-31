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
#include <atomic>
#include <Fact/Fact.h>
#include <QtConcurrent>
#include <QtCore>
#include <QtLocation>

class KmlOverlay : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate center READ getCenter NOTIFY centerChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(int zonesCount READ zonesCount NOTIFY dataChanged)

public:
    explicit KmlOverlay(Fact *parent = nullptr);

    Fact *f_open;
    Fact *f_visible;
    Fact *f_opacity;

    QGeoCoordinate getCenter() const;
    bool loading() const { return m_loading; }
    int zonesCount() const;

    // immutable snapshot of the loaded zones, shared with the render thread
    KmlDataSetPtr dataSet() const { return m_data; }

private:
    struct ImportResult
    {
        KmlDataSetPtr data;
        QString error;
    };

    QGeoCoordinate m_center;
    KmlDataSetPtr m_data;

    QFutureWatcher<ImportResult> m_importWatcher;
    QTimer m_progressTimer;
    QSharedPointer<std::atomic<int>> m_parseProgress; // written by the import thread
    bool m_loading{false};

    void setLoading(bool loading);

private slots:
    void onOpenTriggered();
    void onImportFinished();

signals:
    void centerChanged();
    void loadingChanged();
    void dataChanged();
};
