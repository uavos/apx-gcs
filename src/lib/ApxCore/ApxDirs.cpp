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
#include "ApxDirs.h"
//=============================================================================
QDir ApxDirs::res()
{
#ifdef __ANDROID__
    const QString hpath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir("assets:/data/");
#else
    return QDir(QCoreApplication::applicationDirPath() + "/"
                + RELATIVE_DATA_PATH); //+"/../Resources");
#endif
}

QDir ApxDirs::user()
{
#ifdef __ANDROID__
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/.gcu");
#else
    return QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                    .absoluteFilePath("UAVOS"));
#endif
}

QDir ApxDirs::plugins()
{
    return QDir(QCoreApplication::applicationDirPath() + "/"
                + RELATIVE_PLUGIN_PATH); //"/../Plugins/gcs");
}

QDir ApxDirs::userPlugins()
{
    return QDir(user().absoluteFilePath("Plugins"));
}

QDir ApxDirs::firmware()
{
    return QDir(ApxDirs::user().absoluteFilePath("Firmware"));
}

QDir ApxDirs::prefs()
{
    return QDir(ApxDirs::user().absoluteFilePath("Preferences"));
}

QDir ApxDirs::lang()
{
    return QDir(QCoreApplication::applicationDirPath() + "/../Localization/gcs");
}

QDir ApxDirs::missions()
{
    return QDir(user().absoluteFilePath("Missions"));
}

QDir ApxDirs::configs()
{
    return QDir(user().absoluteFilePath("Configs"));
}

QDir ApxDirs::scripts()
{
    return QDir(user().absoluteFilePath("Scripts"));
}

QDir ApxDirs::db()
{
    return QDir(user().absoluteFilePath("Data"));
}

QDir ApxDirs::logs()
{
    return QDir(user().absoluteFilePath("Logs"));
}

QDir ApxDirs::video()
{
    return QDir(user().absoluteFilePath("Video"));
}

QDir ApxDirs::images()
{
    return QDir(user().absoluteFilePath("Images"));
}

//-------------------------------------------
//HELPERS

bool ApxDirs::copyPath(QString sourceDir, QString destinationDir)
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

    originDirectory.mkpath(destinationDir);

    foreach (QString directoryName, originDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString destinationPath = destinationDir + "/" + directoryName;
        originDirectory.mkpath(destinationPath);
        copyPath(sourceDir + "/" + directoryName, destinationPath);
    }

    foreach (QString fileName, originDirectory.entryList(QDir::Files)) {
        QFile::copy(sourceDir + "/" + fileName, destinationDir + "/" + fileName);
    }

    /*! Possible race-condition mitigation? */
    QDir finalDestination(destinationDir);
    finalDestination.refresh();

    if (finalDestination.exists()) {
        return true;
    }

    return false;
}
