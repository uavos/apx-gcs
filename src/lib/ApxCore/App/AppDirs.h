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

#include <QFileDialog>
#include <QtCore>

namespace AppDirs {

QDir res();
QDir user();
QDir libs();
QDir plugins();
QDir userPlugins();
QDir firmware();
QDir prefs();
QDir missions();
QDir configs();
QDir scripts();
QDir db();
QDir logs();
QDir video();
QDir images();
QDir configBackups();

//helpers
namespace utils {

QByteArray getFileHash(const QFileInfo &fileInfo);

QFileInfo newerFile(const QFileInfo &info1, const QFileInfo &info2);

template<typename... Args>
QFileInfo newerFile(const QFileInfo &firstInfo, Args &&...infos);

template<typename... Args>
bool isHashEqual(const QFileInfo &firstHash, Args &&...hashes);

bool backupFile(const QFileInfo &fileToBackupInfo, const QString &backupDirPath);

bool simpleCopy(const QFileInfo &srcInfo, const QFileInfo &destInfo, const bool overwrite = true);

bool createDir(const QDir &directory);

bool isFileNeedOverwrite(const QFileInfo &srcInfo, const QFileInfo &destInfo);

bool hasSomeHash(const QFileInfo &sourceFileInfo, const QDir &targetDir);

bool copyFile(QString sourceFilePath, QString destinationPath, QString backupDirPath = "");

bool copyDir(QString sourceDirPath,
             QString destinationDirPath,
             QString fileExtension = "*",
             QString backupDirPath = "");
} // namespace utils

void copyPath(const QString &sourcePath,
              const QString &destinationPath,
              const QString &fileExtension = "*",
              const QString &backupDirPath = "");

}; // namespace AppDirs
