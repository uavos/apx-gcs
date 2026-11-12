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
#include "AppPluginsGithubRepo.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <App/AppPlugins.h>

#include <quazip/JlCompress.h>

AppPluginsGithubRepo::AppPluginsGithubRepo(Fact *parent, const QJsonObject &info)
    : Fact(parent,
           info.value("repo").toString().toLower(),
           QString("%1/%2").arg(info.value("owner").toString(), info.value("repo").toString()),
           info.value("url").toString(),
           Group,
           "source-repository")
    , _owner(info.value("owner").toString())
    , _repo(info.value("repo").toString())
    , _info(info)
    , _gh(QString("%1/%2").arg(_owner, _repo), this)
{
    init();
}

AppPluginsGithubRepo::AppPluginsGithubRepo(Fact *parent, const QString &owner, const QString &repo)
    : Fact(parent,
           repo.toLower(),
           QString("%1/%2").arg(owner, repo),
           QString("https://github.com/%1/%2").arg(owner, repo),
           Group,
           "source-repository")
    , _owner(owner)
    , _repo(repo)
    , _gh(QString("%1/%2").arg(_owner, _repo), this)
{
    init();
}

void AppPluginsGithubRepo::init()
{
    f_library = new Fact(this, "library", tr("Library"), tr("Installed plugin library"));

    f_update = new Fact(this,
                        "update",
                        tr("Update"),
                        tr("Download the latest release for this version"),
                        Action | Apply,
                        "update");
    connect(f_update, &Fact::triggered, this, &AppPluginsGithubRepo::install);

    f_remove = new Fact(this,
                        "remove",
                        tr("Remove"),
                        tr("Delete the plugin folder"),
                        Action | Remove | CloseOnTrigger,
                        "delete");

    // GithubReleases
    connect(&_gh, &GithubReleases::releasesInfo, this, &AppPluginsGithubRepo::releasesInfo);
    connect(&_gh, &GithubReleases::downloadFinished, this, &AppPluginsGithubRepo::downloadFinished);
    connect(&_gh, &GithubReleases::downloadProgress, this, [this](qint64 rcv, qint64 total) {
        if (total > 0)
            setProgress(static_cast<int>(rcv * 100 / total));
    });
    connect(&_gh, &GithubReleases::finished, this, [this]() { setProgress(-1); });
    connect(&_gh, &GithubReleases::error, this, [this](QString msg) {
        _installAfterCheck = false;
        fail(msg);
    });

    updateStatus();
}

QString AppPluginsGithubRepo::url() const
{
    return QString("https://github.com/%1/%2").arg(_owner, _repo);
}

QDir AppPluginsGithubRepo::dir() const
{
    return QDir(AppDirs::githubPlugins().absoluteFilePath(_repo));
}

QString AppPluginsGithubRepo::infoPath() const
{
    return dir().absoluteFilePath(_repo + ".json");
}

QVersionNumber AppPluginsGithubRepo::installedVersion() const
{
    return AppPlugins::pluginVersion(installedTag());
}

bool AppPluginsGithubRepo::isCompatible() const
{
    return isInstalled() && AppPlugins::isCompatible(installedVersion());
}

bool AppPluginsGithubRepo::hasUpdate() const
{
    if (_latestTag.isEmpty())
        return false;
    if (!isCompatible())
        return true;
    return _latestVersion > installedVersion();
}

QString AppPluginsGithubRepo::status() const
{
    if (!_checked)
        return isInstalled() ? QString() : tr("Not installed");
    if (_latestTag.isEmpty())
        return tr("Not available for version %1").arg(App::version());
    if (!isInstalled())
        return tr("Release available").append(": ") + _latestTag;
    if (!isCompatible())
        return tr("Not compatible with version %1, reinstall %2").arg(App::version(), _latestTag);
    if (hasUpdate())
        return tr("Update available").append(": ") + _latestTag;
    return tr("Up to date");
}

void AppPluginsGithubRepo::updateStatus()
{
    const QString tag = installedTag();
    setValue(tag);
    QString lib = _info.value("library").toString();
    if (lib.isEmpty())
        lib = _info.value("asset").toString();
    f_library->setValue(isInstalled() ? lib + " " + tag : QString());

    const QString st = status();
    setDescr(st.isEmpty() ? url() : st);

    // the update action is disabled when the installed release is the latest for this version
    f_update->setEnabled(!_checked || hasUpdate());
    f_update->setDescr(_checked ? st : tr("Download the latest release for this version"));
}

// platform assets

QStringList AppPluginsGithubRepo::platformTags()
{
    QString os = QSysInfo::kernelType(); // linux, darwin, winnt
    if (os == "darwin")
        os = "macos";
    else if (os == "winnt")
        os = "windows";
    const QString arch = QSysInfo::currentCpuArchitecture(); // x86_64, arm64, i386
    QStringList tags;
    tags << os + "-" + arch;
    if (os == "macos")
        tags << os + "-universal";
    return tags;
}

QString AppPluginsGithubRepo::pickAsset(const QJsonArray &assets,
                                        QString &assetName,
                                        QString &pluginName)
{
    // zip archive with the library inside is preferred, bare library accepted
    static const QStringList exts{"zip", "so", "dylib", "gcs"};
    for (const auto &tag : platformTags()) {
        for (const auto &ext : exts) {
            const QString suffix = "-" + tag + "." + ext;
            for (const auto &v : assets) {
                const auto a = v.toObject();
                const QString name = a.value("name").toString();
                if (!name.endsWith(suffix, Qt::CaseInsensitive) || name.size() <= suffix.size())
                    continue;
                assetName = name;
                pluginName = AppPlugins::pluginName(name.left(name.size() - suffix.size()));
                return a.value("browser_download_url").toString();
            }
        }
    }
    return {};
}

// check

void AppPluginsGithubRepo::check()
{
    if (busy())
        return;
    setProgress(0);
    _gh.requestReleases();
}

void AppPluginsGithubRepo::releasesInfo(QJsonArray releases)
{
    _checked = true;
    _latestVersion = QVersionNumber();
    _latestTag.clear();
    _latestAsset.clear();
    _latestPlugin.clear();
    _latestUrl.clear();

    // the latest release built for the application version (11.2.32.x) with a library for this platform
    for (const auto &v : releases) {
        const auto r = v.toObject();
        if (r.value("draft").toBool() || r.value("prerelease").toBool())
            continue;
        const QString tag = r.value("tag_name").toString();
        const auto version = AppPlugins::pluginVersion(tag);
        if (!AppPlugins::isCompatible(version))
            continue;
        if (!_latestVersion.isNull() && version <= _latestVersion)
            continue;
        QString asset, plugin;
        const QString url = pickAsset(r.value("assets").toArray(), asset, plugin);
        if (url.isEmpty())
            continue;
        _latestVersion = version;
        _latestTag = tag;
        _latestAsset = asset;
        _latestPlugin = plugin;
        _latestUrl = url;
    }

    updateStatus();
    apxConsole() << title().append(':') << status();
    emit checked();

    if (!_installAfterCheck)
        return;
    _installAfterCheck = false;
    if (hasUpdate())
        install();
    else
        apxMsg() << title().append(':') << status();
}

// install

void AppPluginsGithubRepo::install()
{
    if (busy()) {
        apxMsgW() << title().append(':') << tr("Busy");
        return;
    }
    if (!_checked) {
        _installAfterCheck = true;
        check();
        return;
    }
    if (!hasUpdate()) {
        apxMsg() << title().append(':') << status();
        return;
    }
    if (!dir().mkpath(".")) {
        fail(tr("Can't create folder").append(": ") + dir().absolutePath());
        return;
    }
    setProgress(0);
    apxMsg() << title().append(':') << tr("Downloading") << _latestAsset << _latestTag;
    _gh.requestDownload(QUrl(_latestUrl));
}

void AppPluginsGithubRepo::downloadFinished(QString assetName, QFile *data)
{
    if (!assetName.isEmpty() && assetName != _latestAsset) {
        fail(tr("Unknown response").append(": ") + assetName);
        return;
    }
    const QDir d = dir();
    const QString ext = QFileInfo(_latestAsset).suffix().toLower();
    QString libPath;
    if (ext == "zip") {
        // replace the libraries with the archive content
        for (const auto &f : AppPlugins::findLibPlugins(d))
            QFile::remove(f);
        const auto files = JlCompress::extractDir(data->fileName(), d.absolutePath());
        if (files.isEmpty()) {
            fail(tr("Can't unpack").append(": ") + _latestAsset);
            return;
        }
        const auto libs = AppPlugins::findLibPlugins(d);
        if (libs.isEmpty()) {
            fail(tr("No plugin library in").append(": ") + _latestAsset);
            return;
        }
        libPath = libs.first();
    } else {
        // bare library: <Name>.so.part is renamed when complete
        libPath = d.absoluteFilePath(_latestPlugin + "." + ext);
        const QString partPath = libPath + ".part";
        QFile::remove(partPath);
        if (!data->copy(partPath)) {
            fail(tr("Can't write").append(": ") + partPath);
            return;
        }
        QFile::remove(libPath);
        if (!QFile::rename(partPath, libPath)) {
            QFile::remove(partPath);
            fail(tr("Can't write").append(": ") + libPath);
            return;
        }
    }

    _info = QJsonObject();
    _info["url"] = url();
    _info["owner"] = _owner;
    _info["repo"] = _repo;
    _info["tag"] = _latestTag;
    _info["asset"] = _latestAsset;
    _info["library"] = QFileInfo(libPath).fileName();
    _info["platform"] = platformTags().first();
    _info["plugins"] = QJsonArray{AppPlugins::pluginName(libPath)};
    _info["installed"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!writeInfo())
        apxMsgW() << title().append(':') << tr("Can't write") << infoPath();

    setProgress(-1);
    updateStatus();
    apxMsg() << title().append(':') << tr("Installed") << QFileInfo(libPath).fileName()
             << _latestTag;
    emit installed(libPath);
}

bool AppPluginsGithubRepo::writeInfo() const
{
    QFile f(infoPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(QJsonDocument(_info).toJson(QJsonDocument::Indented));
    return true;
}

void AppPluginsGithubRepo::abort()
{
    _installAfterCheck = false;
    _gh.abort();
    setProgress(-1);
}

void AppPluginsGithubRepo::fail(const QString &msg)
{
    setProgress(-1);
    apxMsgW() << title().append(':') << msg;
    emit failed(msg);
}
