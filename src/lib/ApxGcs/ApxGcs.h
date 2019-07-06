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
#ifndef ApxGcs_H
#define ApxGcs_H
#include <ApxApp.h>

#include <ApxDirs.h>
#include <ApxLog.h>
#include <ApxApp.h>
#include <App/AppSettings.h>

#include <Database/Database.h>
#include <Datalink/Datalink.h>

#include <Vehicles/Vehicles.h>
#include <Protocols/ApxProtocol.h>
//=============================================================================
class ApxGcs : public ApxApp
{
    Q_OBJECT
    Q_ENUMS(FileType)

public:
    explicit ApxGcs(int &argc, char **argv, const QString &name, const QUrl &url);
    static ApxGcs *instance() { return _instance; }

    enum FileType {
        UnknownFile = 0,
        TelemetryFile,
        ConfigFile,
        FirmwareFile,
    };
    Q_ENUM(FileType)

    ApxProtocol *protocol;

    Datalink *f_datalink;

private:
    static ApxGcs *_instance;

protected:
    void loadServices();

public slots:
    void openFile(ApxGcs::FileType type = FileType::UnknownFile);

signals:
    void fileOpenRequest(QString fileName);
};
//=============================================================================
#endif
