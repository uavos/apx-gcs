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
#include "ShareImport.h"

#include <App/AppLog.h>

ShareImport::ShareImport(QString name, QString type, QObject *parent)
    : QObject(parent)
    , _name(name)
    , _type(type)
{}

QByteArray ShareImport::loadData(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot read file").append(":") << fileName
                  << QString("(%1)").arg(file.errorString());
        return QByteArray();
    }
    return file.readAll();
}

/*void Share::_syncTemplates()
{
    //import default data from resources
    QSettings sx;
    sx.beginGroup("templates_update");
    QStringList importedFiles = sx.value(fileTypes.first()).toStringList();
    QFileInfoList fiSrcList(QDir(AppDirs::res().absoluteFilePath("templates/share"),
                                 QString("*.%1").arg(fileTypes.first()))
                                .entryInfoList());

    bool updated = false;
    foreach (QFileInfo fi, fiSrcList) {
        if (importedFiles.contains(fi.completeBaseName()))
            continue;
        importedFiles.append(fi.completeBaseName());
        updated = true;

        qDebug() << "template update:" << fi.absoluteFilePath();
        ShareXmlImport *req = importRequest(fi.filePath());
        if (req)
            req->exec();
    }
    if (updated) {
        importedFiles.sort();
        importedFiles.removeDuplicates();
        sx.setValue(fileTypes.first(), importedFiles);
    }
}*/
