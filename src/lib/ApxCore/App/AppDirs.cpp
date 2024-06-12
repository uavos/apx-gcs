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

namespace AppDirs {

QDir res()
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

QDir user()
{
#ifdef __ANDROID__
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/.gcu");
#else
    return QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                    .absoluteFilePath("UAVOS"));
#endif
}

QDir libs()
{
    QDir dir = QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_LIBS_PATH);
    if (dir.exists())
        return dir;
    dir = QDir(QCoreApplication::applicationDirPath() + "/../lib"); // build from sources
    if (dir.exists())
        return dir;
    return QDir();
}

QDir plugins()
{
    QDir dir = QDir(QCoreApplication::applicationDirPath() + "/" + RELATIVE_PLUGINS_PATH);
    if (dir.exists())
        return dir;
    dir = QDir(QCoreApplication::applicationDirPath() + "/../plugins"); // build from sources
    if (dir.exists())
        return dir;
    return QDir();
}

QDir userPlugins()
{
    return QDir(user().absoluteFilePath("Plugins"));
}

QDir firmware()
{
    return QDir(user().absoluteFilePath("Firmware"));
}

QDir prefs()
{
    return QDir(user().absoluteFilePath("Preferences"));
}

QDir missions()
{
    return QDir(user().absoluteFilePath("Missions"));
}

QDir configs()
{
    return QDir(user().absoluteFilePath("Configs"));
}

QDir configBackups()
{
    return QDir(configs().absoluteFilePath(".backup"));
}

QDir scripts()
{
    return QDir(user().absoluteFilePath("Scripts"));
}

QDir db()
{
    return QDir(user().absoluteFilePath("Data"));
}

QDir logs()
{
    return QDir(user().absoluteFilePath("Logs"));
}

QDir video()
{
    return QDir(user().absoluteFilePath("Video"));
}

QDir images()
{
    return QDir(user().absoluteFilePath("Images"));
}

//-------------------------------------------
//HELPERS
namespace utils {

QByteArray getFileHash(const QFileInfo &fileInfo)
{
    QFile file(fileInfo.absoluteFilePath());
    file.open(QIODevice::ReadOnly);
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    return hash.result();
}

QFileInfo newerFile(const QFileInfo &info1, const QFileInfo &info2)
{
    return (info1.lastModified() < info2.lastModified() ? info2 : info1);
}

template<typename... Args>
QFileInfo newerFile(const QFileInfo &firstInfo, Args &&...infos)
{
    return newerFile(firstInfo, newerFile(std::forward<Args>(infos)...));
}

template<typename... Args>
bool isHashEqual(const QFileInfo &firstHash, Args &&...hashes)
{
    return (... && (getFileHash(std::forward<Args>(hashes)) == getFileHash(firstHash)));
}

bool backupFile(const QFileInfo &fileToBackupInfo, const QString &backupDirPath)
{
    QDir backupDir(backupDirPath);

    if (!createDir(backupDir))
        return false;

    if (hasSomeHash(fileToBackupInfo, backupDir))
        return true;

    QStringList nameFilter;
    nameFilter << fileToBackupInfo.baseName() + "." + fileToBackupInfo.completeSuffix() + ".*";
    backupDir.setNameFilters(nameFilter);

    QFileInfoList fileInfos = backupDir.entryInfoList(QDir::NoFilter, QDir::Name);
    int index = 1;
    if (!fileInfos.isEmpty()) {
        QFileInfo lastBackup = fileInfos.back();
        index = lastBackup.suffix().toInt() + 1;
    }

    QFileInfo backupPathInfo(backupDirPath + "/" + fileToBackupInfo.fileName() + "."
                             + QString::number(index));

    return simpleCopy(fileToBackupInfo, backupPathInfo, false);
}

bool simpleCopy(const QFileInfo &srcInfo, const QFileInfo &destInfo, const bool overwrite)
{
    if (overwrite && destInfo.exists())
        QFile::remove(destInfo.absoluteFilePath());

    if (QFile::copy(srcInfo.absoluteFilePath(), destInfo.absoluteFilePath()))
        return true;
    qDebug() << "Error in copying from " << srcInfo.absoluteFilePath() << " to "
             << destInfo.absoluteFilePath();
    return false;
}

bool createDir(const QDir &directory)
{
    if (!directory.exists() && !directory.mkpath(".")) {
        qDebug() << "Can't create " << directory.path() << " directory";
        return false;
    }
    return true;
}

bool isFileNeedOverwrite(const QFileInfo &srcInfo, const QFileInfo &destInfo)
{
    return (newerFile(srcInfo, destInfo) == srcInfo && !isHashEqual(srcInfo, destInfo));
}

bool hasSomeHash(const QFileInfo &sourceFileInfo, const QDir &targetDir)
{
    QStringList nameFilter(sourceFileInfo.fileName() + ".*");
    QFileInfoList infoList = targetDir.entryInfoList(nameFilter);

    QByteArray hash = getFileHash(sourceFileInfo);
    return std::any_of(std::cbegin(infoList), std::cend(infoList), [&hash](const QFileInfo &info) {
        return hash == getFileHash(info);
    });
};

bool copyFile(QString sourceFilePath, QString destinationPath, QString backupDirPath)
{
    if (QFileInfo(destinationPath).isDir())
        destinationPath = destinationPath + "/" + QFileInfo(sourceFilePath).fileName();

    QFileInfo destinationFileInfo(destinationPath);

    const bool isDestinationFileExist = destinationFileInfo.exists();

    if (!isDestinationFileExist && !createDir(destinationFileInfo.absolutePath()))
        return false;

    const bool isBackupPathProvided = !backupDirPath.isEmpty();
    const bool overwriteFile = isFileNeedOverwrite(sourceFilePath, destinationPath);

    if (isBackupPathProvided && isDestinationFileExist && overwriteFile
        && !backupFile(destinationPath, backupDirPath))
        return false;

    const bool isFileNeedCopy = !isDestinationFileExist
                                || (isDestinationFileExist && overwriteFile);

    return isFileNeedCopy && !simpleCopy(sourceFilePath, destinationPath) ? false : true;
}

bool copyDir(QString sourceDirPath,
             QString destinationDirPath,
             QString fileExtension,
             QString backupDirPath)
{
    if (!utils::createDir(destinationDirPath))
        return false;

    const bool isBackupPathProvided = !backupDirPath.isEmpty();
    if (isBackupPathProvided && !utils::createDir(backupDirPath))
        return false;

    QDirIterator it(sourceDirPath,
                    QStringList() << "*." + fileExtension,
                    QDir::Filters(QDir::AllEntries | QDir::NoDotAndDotDot),
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString sourcefilePath = it.next();
        QString fileRelativePath = sourcefilePath;

        fileRelativePath.remove(sourceDirPath + "/");

        QString destinationFilePath = destinationDirPath + "/" + fileRelativePath;
        QFileInfo destinationFileInfo(destinationFilePath);

        QString backupFilePath = backupDirPath + "/" + fileRelativePath;
        QFileInfo backupFileInfo(backupFilePath);

        if (!createDir(destinationFileInfo.absolutePath()))
            continue;

        copyFile(sourcefilePath,
                 destinationFilePath,
                 isBackupPathProvided ? backupFileInfo.absolutePath() : "");
    }

    return false;
}

} // namespace utils

void copyPath(const QString &sourcePath,
              const QString &destinationPath,
              const QString &fileExtension,
              const QString &backupDirPath)
{
    QFileInfo sourceInfo(sourcePath);

    if (!sourceInfo.exists()) {
        qDebug() << sourcePath << " path doesn't exist";
        return;
    }

    if (sourceInfo.isFile()) {
        utils::copyFile(sourcePath, destinationPath, backupDirPath);
        return;
    }

    utils::copyDir(sourcePath, destinationPath, fileExtension, backupDirPath);
    return;
}

} // namespace AppDirs
