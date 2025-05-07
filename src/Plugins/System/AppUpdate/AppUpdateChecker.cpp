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
#include "AppUpdateChecker.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppGcs.h>

#include <QDesktopServices>

AppUpdateChecker::AppUpdateChecker(Fact *parent)
    : Fact(parent, "checker", "", tr("Current version: ").append(App::version()), Fact::Group)
{
    setOpt("page", "qrc:/" PLUGIN_NAME "/AppUpdate.qml");

    QSettings sx;
    auto v = sx.value("dist_file").toString();
    if (QFile::exists(v)) {
        QFile::remove(v);
        apxMsg() << tr("Removed old update file");
    }
    sx.remove("dist_file");
    sx.sync();

    f_www = new Fact(this, "www", tr("Visit site"), "", Action, "git");
    connect(f_www, &Fact::triggered, this, [this]() {
        QDesktopServices::openUrl(
            _releaseUrl.isValid() ? _releaseUrl
                                  : QString("https://github.com/%1/releases").arg(_gh.repoName()));
    });

    f_upgrade = new Fact(this, "upgrade", tr("Upgrade"), title(), Action | Apply, "download");
    connect(f_upgrade, &Fact::triggered, this, &AppUpdateChecker::installUpdate);
    f_upgrade->setEnabled(false);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop upgrade"), Action | Stop, "stop");
    connect(f_stop, &Fact::triggered, this, &AppUpdateChecker::abort);
    f_stop->setVisible(false);

    // connect GithubReleases
    connect(&_gh, &GithubReleases::latestVersionInfo, this, &AppUpdateChecker::latestVersionInfo);
    connect(&_gh, &GithubReleases::releaseInfo, this, &AppUpdateChecker::releaseInfo);
    connect(&_gh, &GithubReleases::downloadProgress, this, &AppUpdateChecker::downloadProgress);
    connect(&_gh, &GithubReleases::downloadFinished, this, &AppUpdateChecker::downloadFinished);
    connect(&_gh, &GithubReleases::finished, this, [this]() { setProgress(-1); });
    connect(&_gh, &GithubReleases::error, this, [this](QString msg) {
        apxMsgW() << parentFact()->title().append(':') << msg;
    });

    connect(this, &Fact::progressChanged, this, [this]() {
        bool busy = progress() >= 0;
        f_upgrade->setVisible(!busy);
        f_stop->setVisible(busy);
    });

    connect(this, &AppUpdateChecker::releaseInfoChanged, this, &AppUpdateChecker::updateStatus);
    updateStatus();
}

void AppUpdateChecker::updateStatus()
{
    if (_releaseVersion.isNull()) {
        setEnabled(false);
        setTitle(tr("Waiting for updates"));
        return;
    }
    // repo version available
    setProgress(-1);

    // compare versions
    auto current = QVersionNumber::fromString(App::version());
    if (current < _releaseVersion) {
        setEnabled(true);
        setTitle(tr("Update available: ").append(_releaseVersion.toString()));
#ifdef Q_OS_MACOS
        f_upgrade->setEnabled(true);
#endif
        trigger();
    } else {
        setEnabled(false);
        setTitle(tr("No updates available"));
        f_upgrade->setEnabled(false);
    }
}

void AppUpdateChecker::abort()
{
    _gh.abort();
}

void AppUpdateChecker::checkForUpdates()
{
    setProgress(0);
    _gh.requestLatestVersionInfo();
}

void AppUpdateChecker::installUpdate()
{
    if (_releaseVersion.isNull())
        return;

    // check for already downloaded asset
    QDir dir(AppDirs::user());
    auto assetFilePath = dir.absoluteFilePath(_assetName);
    if (QFile::exists(assetFilePath)) {
        reinstallApplication(assetFilePath);
        return;
    }

    // download new asset
    _gh.requestDownload(_assetUrl);
}

void AppUpdateChecker::latestVersionInfo(QVersionNumber version, QString tag)
{
    _releaseVersion = version;
    _releaseName.clear();
    _releaseNotes.clear();
    _assetName.clear();
    _assetUrl.clear();
    _releaseUrl.clear();
    emit releaseInfoChanged();

    auto v_current = QVersionNumber::fromString(QCoreApplication::applicationVersion());
    if (v_current >= version) {
        qDebug() << "no updates:" << v_current << ">=" << version;
    } else {
        apxMsg() << tr("Update available").append(':') << version.toString();
        _gh.requestReleaseInfo(tag);
    }
}

void AppUpdateChecker::releaseInfo(QJsonDocument json)
{
    while (json["assets"].toArray().count() > 0) {
        if (json["author"]["login"].toString() != "uavinda")
            break;

        //find asset
        _releaseName = json["name"].toString();
        _releaseNotes = json["body"].toString();
        _releaseUrl = json["html_url"].toString();

        for (auto asset : json["assets"].toArray()) {
            auto jo = asset.toObject();
            auto name = jo["name"].toString();

            // parse filename components
            // APX_Ground_Control-11.1.15-macos-x86_64.dmg
            auto fi = QFileInfo(name);
            auto parts = fi.completeBaseName().split('-');

            if (parts.size() != 4)
                continue;
            if (parts[0] != "APX_Ground_Control")
                continue;
            if (parts[1] != _releaseVersion.toString())
                continue;

#if defined(Q_OS_MAC)
            if (fi.suffix() != "dmg")
                continue;
            if (parts[2] != "macos")
                continue;
            if (parts[3] != "universal" && parts[3] != QSysInfo::currentCpuArchitecture())
                continue;
#elif defined(Q_OS_LINUX)
            if (fi.suffix() != "AppImage")
                continue;
            if (parts[2] != "linux")
                continue;
            if (parts[3] != QSysInfo::currentCpuArchitecture())
                continue;
#else
#error "Unknown platform"
#endif

            _assetName = name;
            _assetUrl = jo["browser_download_url"].toString();
            break;
        }
        emit releaseInfoChanged();

        if (_assetName.isEmpty())
            break;

        // downloadable asset found
        qDebug() << "asset:" << _assetName;
        return;
    }
    apxMsgW() << tr("Update not available");
    qWarning() << json;
}

void AppUpdateChecker::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        setProgress((bytesReceived * 100) / bytesTotal);
    }
}

void AppUpdateChecker::downloadFinished(QString assetName, QFile *data)
{
    if (assetName != _assetName) {
        apxMsgW() << tr("Unknown response") << assetName;
        return;
    }

    QDir dir(AppDirs::user());
    dir.mkpath(".");
    auto newFileName = dir.absoluteFilePath(assetName);

    if (!data->copy(newFileName)) {
        apxMsgW() << tr("Can't copy file:") << newFileName;
        return;
    }

    qDebug() << "downloaded" << newFileName << QFileInfo(newFileName).size();

    QSettings sx;
    sx.setValue("dist_file", newFileName);
    sx.sync();

    reinstallApplication(newFileName);
}

void AppUpdateChecker::reinstallApplication(QString filePath)
{
#ifdef Q_OS_MACOS
    if (!App::bundle()) {
        apxMsgW() << tr("Can't find app bundle");
        return;
    }

    // hdiutil
    setProgress(0);
    _process = new QProcess(this);
    connect(_process, &QProcess::finished, this, &AppUpdateChecker::hdutilFinished);

    _process->start("hdiutil",
                    QStringList() << "attach"
                                  << "-nobrowse"
                                  << "-readonly" << filePath);
#endif
}

void AppUpdateChecker::hdutilFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
#ifdef Q_OS_MACOS
    setProgress(-1);
    _process->deleteLater();

    if (exitCode != 0) {
        apxMsgW() << tr("Can't mount disk image");
        return;
    }

    // mounted
    auto appFileName = QCoreApplication::applicationFilePath();
    auto appDir = QFileInfo(appFileName).absoluteDir();
    appDir.cdUp();                  // Contents
    appDir.cdUp();                  // .app
    appFileName = appDir.dirName(); // bundle name with extension

    // find mount point
    auto out = QString::fromUtf8(_process->readAllStandardOutput());
    static QRegularExpression re("(/Volumes/[^\\n]+)");
    auto match = re.match(out);
    QString mountVolume, mountPath;
    if (match.hasMatch()) {
        mountVolume = match.captured(1);
        mountPath = QString("%1/%2").arg(mountVolume, appFileName);
    }

    if (!QDir(mountPath).exists()) {
        apxMsgW() << tr("Can't find source bundle");
        if (QDir(mountVolume).exists()) {
            QProcess::startDetached("hdiutil", QStringList() << "detach" << mountVolume);
        }
        return;
    }

    qDebug() << "reinstall from" << mountPath << "to" << appDir;

    // stop app
    App::instance()->hide();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
    QCoreApplication::quit();

    if (appDir.removeRecursively()) {
        qDebug() << "Removed" << appDir;
    } else {
        qWarning() << "Can't remove" << appDir;
    }

    // copy
    auto fromDir = mountPath;
    auto toDir = appDir.absolutePath();

    QDirIterator it(fromDir,
                    QDir::Dirs | QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    QDir dir(fromDir);
    const int absSourcePathLength = dir.absoluteFilePath(fromDir).length();
    while (it.hasNext()) {
        it.next();
        const auto fileInfo = it.fileInfo();

        const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
        const QString constructedAbsolutePath = toDir + subPathStructure;

        if (fileInfo.isDir()) {
            //Create directory in target folder
            dir.mkpath(constructedAbsolutePath);

        } else if (fileInfo.isSymLink()) {
            //Copy symbolic link to target directory
            QFile::remove(constructedAbsolutePath);
            QString linkTarget = fileInfo.dir().relativeFilePath(fileInfo.symLinkTarget());
            if (!QFile::link(linkTarget, constructedAbsolutePath)) {
                qWarning() << "Can't create symlink:" << constructedAbsolutePath;
            } else {
                qDebug() << "Created symlink:" << constructedAbsolutePath;
            }

        } else if (fileInfo.isFile()) {
            //Copy File to target directory
            //Remove file at target location, if it exists, or QFile::copy will fail
            QFile::remove(constructedAbsolutePath);
            if (!QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath)) {
                qWarning() << "Can't copy file:" << constructedAbsolutePath;
            } else {
                qDebug() << "Copied file:" << constructedAbsolutePath;
            }
        }
    }

    // unmount
    if (QProcess::execute("hdiutil", QStringList() << "detach" << mountVolume) != 0) {
        qWarning() << tr("Can't unmount disk image");
    }

    // restart
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
#endif
}
