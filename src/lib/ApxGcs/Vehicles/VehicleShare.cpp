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
#include "VehicleShare.h"
#include "Vehicle.h"

#include <Nodes/Nodes.h>

#include <App/AppDirs.h>
#include <App/AppLog.h>

VehicleShare::VehicleShare(Vehicle *vehicle, Fact *parent, Flags flags)
    : Share(parent, "vehicle", tr("Vehicle configuration"), AppDirs::configs(), flags)
    , _vehicle(vehicle)
{
    connect(vehicle->f_nodes, &Nodes::validChanged, this, &VehicleShare::updateActions);
    updateActions();
}

QString VehicleShare::getDefaultTitle()
{
    QStringList st;
    st << _vehicle->title();
    st << _vehicle->f_nodes->getConfigTitle();
    return st.join('-');
}
bool VehicleShare::exportRequest(QString format, QString fileName)
{
    if (!saveData(_vehicle->toJsonDocument().toJson(), fileName))
        return false;
    _exported(fileName);
    return true;
}
bool VehicleShare::importRequest(QStringList fileNames)
{
    auto fileName = fileNames.first();
    auto var = parseJsonDocument(loadData(fileName));
    if (var.isNull())
        return false;

    _vehicle->storage()->importVehicleConfig(var.value<QVariantMap>());

    if (QFileInfo(fileName).absoluteDir().absolutePath() == _templatesDir.absolutePath()) {
        // imported
        return true;
    }

    _vehicle->fromVariant(var);
    _imported(fileName);
    return true;
}

void VehicleShare::updateActions()
{
    f_export->setEnabled(_vehicle->f_nodes->valid());
}

void VehicleShare::syncTemplates()
{
    auto format = _importFormats.first();
    QSettings sx;
    sx.beginGroup("templates_update");
    auto importedFiles = sx.value(format).toMap();

    bool updated = false;
    for (auto fi : _templatesDir.entryInfoList()) {
        auto name = fi.completeBaseName();
        auto t_res = fi.lastModified().toMSecsSinceEpoch();
        if (importedFiles.contains(name)) {
            auto t_imp = importedFiles.value(name).toULongLong();
            if (t_res <= t_imp)
                continue;
        }

        qDebug() << "template update:" << fi.fileName();
        if (!importRequest({fi.absoluteFilePath()}))
            continue;

        importedFiles.insert(name, t_res);
        updated = true;
    }
    if (updated) {
        sx.setValue(format, importedFiles);
    }
}
