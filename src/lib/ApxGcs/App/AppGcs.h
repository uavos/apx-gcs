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

#include <App/App.h>

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <App/AppMenu.h>
#include <App/AppSettings.h>

#include <Database/Database.h>
#include <Datalink/Datalink.h>

#include <Vehicles/Vehicles.h>

#include <ApxFw.h>

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

    Datalink *f_datalink;
    AppMenu *f_menu;

    ApxFw *f_apxfw;

    static inline ApxFw *apxfw() { return _instance->f_apxfw; }

private:
    static AppGcs *_instance;

protected:
    void loadServices() override;

public slots:
    void openFile(AppGcs::FileType type = FileType::UnknownFile);

signals:
    void fileOpenRequest(QString fileName);
};
