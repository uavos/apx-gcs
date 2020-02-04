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
#ifndef AppGcs_H
#define AppGcs_H
#include <App/App.h>

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <App/AppMenu.h>
#include <App/AppSettings.h>

#include <Database/Database.h>
#include <Datalink/Datalink.h>

#include <Protocols/ProtocolVehicles.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
class AppGcs : public App
{
    Q_OBJECT
    Q_ENUMS(FileType)

public:
    explicit AppGcs(int &argc, char **argv, const QString &name, const QUrl &_url);
    static AppGcs *instance() { return _instance; }

    enum FileType {
        UnknownFile = 0,
        TelemetryFile,
        ConfigFile,
        FirmwareFile,
    };
    Q_ENUM(FileType)

    ProtocolVehicles *protocol;
    Datalink *f_datalink;
    AppMenu *f_menu;

private:
    static AppGcs *_instance;

protected:
    void loadServices();

public slots:
    void openFile(AppGcs::FileType type = FileType::UnknownFile);

signals:
    void fileOpenRequest(QString fileName);
};
//=============================================================================
#endif
