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
#include "AppGcs.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <QFileDialog>

#include <Database/TelemetryFileFormat.h>
#include <Fleet/Unit.h>
#include <Mission/MissionShare.h>
#include <Mission/UnitMission.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>

AppGcs *AppGcs::_instance = nullptr;
AppGcs::AppGcs(int &argc, char **argv, const QString &name, const QUrl &_url)
    : App(argc, argv, name, _url)
{
    _instance = this;

    connect(this, &App::fileOpenRequest, this, &AppGcs::importFile);
}

void AppGcs::importFile(QString fileName)
{
    const QFileInfo fi(fileName);
    const QString ext = fi.suffix().toLower();
    auto fleet = Fleet::instance();
    if (!fleet)
        return;

    if (!fi.isFile()) {
        apxMsgW() << tr("File not found").append(':') << fi.fileName();
        return;
    }

    Share *share = nullptr;
    if (ext == telemetry::APXTLM_FTYPE || ext == "telemetry") {
        // telemetry is shown by the replay unit
        Unit *unit = Fleet::replay();
        if (unit && unit->f_telemetry) {
            fleet->selectUnit(unit);
            share = unit->f_telemetry->f_share;
        }
    } else if (Unit *unit = fleet->current()) {
        if (ext == "unit" || ext == "nodes")
            share = unit->f_share;
        else if (ext == "mission" || ext == "geofence")
            share = unit->f_mission->f_share;
    }
    if (!share) {
        apxMsgW() << tr("Unsupported file type").append(':') << fi.fileName();
        return;
    }
    if (!share->importRequest({fi.absoluteFilePath()})) {
        apxMsgW() << tr("Can't import").append(':') << fi.fileName();
    }
}

void AppGcs::loadServices()
{
    App::loadServices();

    new Database(f_apx);

    f_datalink = new Datalink(f_apx);

    auto fleet = new Fleet(f_apx, f_datalink->f_protocols);

    f_apxfw = new ApxFw(f_apx);

    // QObject::connect(f_datalink, &Datalink::heartbeat, protocol, &ProtocolFleet::sendHeartbeat);

    fleet->move(f_apx->size());

    f_menu = new AppMenu(f_apx);

    jsync(f_apx);
}

void AppGcs::openFile(AppGcs::FileType type)
{
    QString title;
    QString ftype;
    QDir defaultDir;
    switch (type) {
    default:
        title = tr("Import");
        defaultDir = AppDirs::user();
        break;
    case TelemetryFile:
        title = tr("Import telemetry data");
        ftype = "telemetry";
        defaultDir = AppDirs::user();
        break;
    case ConfigFile:
        title = tr("Import configuration");
        ftype = "nodes";
        defaultDir = AppDirs::configs();
        break;
    case FirmwareFile:
        title = tr("Firmware");
        ftype = "apxfw";
        defaultDir = AppDirs::user();
        break;
    }
    QString settingName = QString("SharePath_%1")
                              .arg(QMetaEnum::fromType<FileType>().valueToKey(type));
    if (!defaultDir.exists())
        defaultDir.mkpath(".");
    QDir dir = QDir(QSettings().value(settingName, defaultDir.canonicalPath()).toString());
    if (!dir.exists())
        dir = defaultDir;
    QFileDialog dlg(nullptr, title, dir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setViewMode(QFileDialog::Detail);
    QStringList filters;
    filters << tr("All supported types") + " (*.telemetry,*.nodes,*.apxfw)"
            << tr("Telemetry") + " (*.telemetry)" << tr("Configuration") + " (*.nodes)"
            << tr("Firmware") + " (*.apxfw)" << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);
    dlg.setDefaultSuffix(ftype);
    if (!ftype.isEmpty())
        dlg.selectNameFilter("*." + ftype);
    if (!(dlg.exec() && dlg.selectedFiles().size() >= 1))
        return;
    QSettings().setValue(settingName, dlg.directory().absolutePath());

    for (const auto fname : dlg.selectedFiles()) {
        emit fileOpenRequest(fname);
    }
}
