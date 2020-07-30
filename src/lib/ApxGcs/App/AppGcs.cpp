/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "AppGcs.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <QFileDialog>
//=============================================================================
AppGcs *AppGcs::_instance = nullptr;
AppGcs::AppGcs(int &argc, char **argv, const QString &name, const QUrl &_url)
    : App(argc, argv, name, _url)
{
    _instance = this;
}
//=============================================================================
void AppGcs::loadServices()
{
    App::loadServices();

    new Database(f_apx);

    protocol = new ProtocolVehicles(f_apx);

    Vehicles *vehicles = new Vehicles(f_apx, protocol);

    f_apxfw = new ApxFw(f_apx);

    //datalink
    f_datalink = new Datalink(f_apx);

    QObject::connect(f_datalink, &Datalink::packetReceived, protocol, &ProtocolVehicles::downlink);
    QObject::connect(protocol, &ProtocolVehicles::uplink, f_datalink, &Datalink::sendPacket);

    QObject::connect(f_datalink, &Datalink::heartbeat, protocol, &ProtocolVehicles::sendHeartbeat);

    vehicles->move(f_apx->size());

    f_menu = new AppMenu(f_apx);

    jsync(f_apx);
}
//=============================================================================
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

    foreach (QString fname, dlg.selectedFiles()) {
        emit fileOpenRequest(fname);
    }
}
//=============================================================================
//=============================================================================
