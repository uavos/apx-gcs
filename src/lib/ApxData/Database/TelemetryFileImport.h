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

#include <QtCore>

#include <Mandala/Mandala.h>
#include <Mandala/MandalaContainers.h>

class TelemetryFileImport : public QTemporaryFile
{
    Q_OBJECT

public:
    explicit TelemetryFileImport(QObject *parent = nullptr);

    bool import_telemetry(QString srcFileName);

    const QJsonObject &info() const { return _info; }
    const QString &src_hash() const { return _src_hash; }

    static QJsonObject import_mission(QXmlStreamReader &xml);
    static QJsonObject import_nodes(QXmlStreamReader &xml);

private:
    QJsonObject _info;
    QString _src_hash;
    QString _srcFileName;

    bool import_telemetry_v11(QXmlStreamReader &xml, QString format);
    bool import_telemetry_v9(QXmlStreamReader &xml);

    // helpers
    static QJsonObject readObject(QXmlStreamReader &xml);

signals:
    void progress(int value);
};
