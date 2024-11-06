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

#include <app_def.h>

QDir AppDirs::res()
{
#ifdef __ANDROID__
    const QString hpath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir("assets:/data/");
#else
    QDir dir = QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_DATA_PATH);
    if (dir.exists())
        return dir;
    QStringList prefix;
    for (int i = 0; i < 4; ++i) {
        prefix.append("..");
        dir = QDir(QCoreApplication::applicationDirPath() + "/" + prefix.join('/')
                   + "/resources"); // build from sources
        if (dir.exists())
            return dir;
        dir = QDir(QCoreApplication::applicationDirPath() + "/" + prefix.join('/')
                   + "/apx-gcs/resources"); // build from sources

        if (dir.exists())
            return dir;
    }
    return QDir();
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
    QDir dir = QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_LIBS_PATH);
    if (dir.exists())
        return dir;
    dir = QDir(QCoreApplication::applicationDirPath() + "/../lib"); // build from sources
    if (dir.exists())
        return dir;
    return QDir();
}

QDir AppDirs::plugins()
{
    QDir dir = QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_PLUGINS_PATH);
    if (dir.exists())
        return dir;
    dir = QDir(QCoreApplication::applicationDirPath() + "/../plugins"); // build from sources
    if (dir.exists())
        return dir;
    return QDir();
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

QDir AppDirs::storage()
{
    return QDir(user().absoluteFilePath("Storage"));
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
    QFileInfo srcInfo(sourceDir);
    QFileInfo destInfo(destinationDir);

    if (srcInfo.isFile()) {
        if (destInfo.isDir()) {
            destInfo = QFileInfo(destInfo.dir().filePath(srcInfo.fileName()));
        }
        if (destInfo.exists()) {
            if (destInfo.lastModified() == srcInfo.lastModified())
                return false;
            QFile::remove(destInfo.absoluteFilePath());
        } else {
            if (!destInfo.dir().exists()) {
                destInfo.dir().mkpath(".");
            }
        }
        QFile::copy(srcInfo.absoluteFilePath(), destInfo.absoluteFilePath());
        return true;
    }

    bool rv = false;

    QDir originDirectory(sourceDir);

    if (!originDirectory.exists()) {
        return rv;
    }

    QDir destinationDirectory(destinationDir);

    /*if(destinationDirectory.exists() && overWriteDirectory)
    {
        destinationDirectory.removeRecursively();
    }*/

    if (!destinationDirectory.exists()) {
        destinationDirectory.mkpath(".");
        rv = true;
    }

    foreach (QString directoryName, originDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString destinationPath = destinationDir + "/" + directoryName;
        //destinationDirectory.mkpath(directoryName);
        copyPath(sourceDir + "/" + directoryName, destinationPath);
    }

    foreach (QString fileName, originDirectory.entryList(QDir::Files)) {
        QFileInfo dest(destinationDir + "/" + fileName);
        QFileInfo src(sourceDir + "/" + fileName);
        if (dest.exists()) {
            if (dest.lastModified() == src.lastModified())
                continue;
            QFile::remove(dest.absoluteFilePath());
        }
        rv = true;
        QFile::copy(src.absoluteFilePath(), dest.absoluteFilePath());
    }

    /*! Possible race-condition mitigation? */
    QDir finalDestination(destinationDir);
    finalDestination.refresh();

    return rv;
}
