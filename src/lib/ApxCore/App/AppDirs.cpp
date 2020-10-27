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
#include "AppDirs.h"
//=============================================================================
QDir AppDirs::res()
{
#ifdef __ANDROID__
    const QString hpath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir("assets:/data/");
#else
    return QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_DATA_PATH);
#endif
}

QDir AppDirs::user()
{
#ifdef __ANDROID__
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/.gcu");
#else
    return QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                    .absoluteFilePath("UAVOS"));
#endif
}

QDir AppDirs::libs()
{
    return QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_LIB_PATH);
}

QDir AppDirs::plugins()
{
    return QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_PLUGIN_PATH);
}

QDir AppDirs::userPlugins()
{
    return QDir(user().absoluteFilePath("Plugins"));
}

QDir AppDirs::firmware()
{
    return QDir(AppDirs::user().absoluteFilePath("Firmware"));
}

QDir AppDirs::prefs()
{
    return QDir(AppDirs::user().absoluteFilePath("Preferences"));
}

QDir AppDirs::missions()
{
    return QDir(user().absoluteFilePath("Missions"));
}

QDir AppDirs::configs()
{
    return QDir(user().absoluteFilePath("Configs"));
}

QDir AppDirs::scripts()
{
    return QDir(user().absoluteFilePath("Scripts"));
}

QDir AppDirs::db()
{
    return QDir(user().absoluteFilePath("Data"));
}

QDir AppDirs::logs()
{
    return QDir(user().absoluteFilePath("Logs"));
}

QDir AppDirs::video()
{
    return QDir(user().absoluteFilePath("Video"));
}

QDir AppDirs::images()
{
    return QDir(user().absoluteFilePath("Images"));
}

//-------------------------------------------
//HELPERS

bool AppDirs::copyPath(QString sourceDir, QString destinationDir)
{
    QDir originDirectory(sourceDir);

    if (!originDirectory.exists()) {
        return false;
    }

    QDir destinationDirectory(destinationDir);

    /*if(destinationDirectory.exists() && overWriteDirectory)
    {
        destinationDirectory.removeRecursively();
    }*/

    destinationDirectory.mkpath(".");

    foreach (QString directoryName, originDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString destinationPath = destinationDir + "/" + directoryName;
        //destinationDirectory.mkpath(directoryName);
        copyPath(sourceDir + "/" + directoryName, destinationPath);
    }

    foreach (QString fileName, originDirectory.entryList(QDir::Files)) {
        QFileInfo fi(destinationDir + "/" + fileName);
        if (fi.exists())
            QFile::remove(fi.absoluteFilePath());
        QFile::copy(sourceDir + "/" + fileName, fi.absoluteFilePath());
    }

    /*! Possible race-condition mitigation? */
    QDir finalDestination(destinationDir);
    finalDestination.refresh();

    if (finalDestination.exists()) {
        return true;
    }

    return false;
}
