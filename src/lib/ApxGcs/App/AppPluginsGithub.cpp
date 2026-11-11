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
#include "AppPluginsGithub.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <App/AppPlugin.h>
#include <App/AppPlugins.h>

AppPluginsGithub::AppPluginsGithub(Fact *parent)
    : Fact(parent,
           "github",
           tr("GitHub plugins"),
           tr("Third party plugins from GitHub repositories"),
           Group,
           "github")
{
    f_url = new Fact(this,
                     "url",
                     tr("Repository"),
                     tr("GitHub repository URL (https://github.com/<owner>/<repo>)"),
                     Text);
    f_url->setSection(tr("Install"));

    f_install = new Fact(this,
                         "install",
                         tr("Install"),
                         tr("Download the plugin library of the latest release"),
                         Action | Apply,
                         "download");
    f_install->setSection(tr("Install"));
    connect(f_install, &Fact::triggered, this, &AppPluginsGithub::install);

    _net.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    updateRepos();
}

bool AppPluginsGithub::parseRepoUrl(const QString &url, QString &owner, QString &repo)
{
    static const QRegularExpression re_url(
        R"(^(?:https?://)?(?:www\.)?github\.com/([\w.-]+)/([\w.-]+?)(?:\.git)?(?:/.*)?$)");
    static const QRegularExpression re_short(R"(^([\w.-]+)/([\w.-]+?)(?:\.git)?$)");
    const QString s = url.trimmed();
    auto m = re_url.match(s);
    if (!m.hasMatch())
        m = re_short.match(s);
    if (!m.hasMatch())
        return false;
    owner = m.captured(1);
    repo = m.captured(2);
    return !owner.isEmpty() && !repo.isEmpty() && !repo.startsWith('.') && !repo.startsWith('-');
}

QStringList AppPluginsGithub::platformTags()
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

QString AppPluginsGithub::pickAsset(const QJsonArray &assets, QString &assetName, QString &libName)
{
    static const QStringList exts{"so", "dylib", "gcs"};
    for (const auto &tag : platformTags()) {
        for (const auto &v : assets) {
            const auto a = v.toObject();
            const QString name = a.value("name").toString();
            const QString ext = QFileInfo(name).suffix().toLower();
            if (!exts.contains(ext))
                continue;
            const QString suffix = "-" + tag + "." + ext;
            if (!name.endsWith(suffix, Qt::CaseInsensitive) || name.size() <= suffix.size())
                continue;
            assetName = name;
            libName = name.left(name.size() - suffix.size()) + "." + ext;
            return a.value("browser_download_url").toString();
        }
    }
    return {};
}

QDir AppPluginsGithub::dir()
{
    return AppDirs::githubPlugins();
}

QString AppPluginsGithub::infoPath(const QString &repo)
{
    return dir().absoluteFilePath(repo + "/" + repo + ".json");
}

// install

void AppPluginsGithub::install()
{
    QString owner, repo;
    if (!parseRepoUrl(f_url->text(), owner, repo)) {
        apxMsgW() << title().append(':') << tr("Invalid repository URL") << f_url->text();
        return;
    }
    if (QFileInfo::exists(dir().absoluteFilePath(repo))) {
        apxMsgW() << title().append(':') << tr("Already installed") << repo;
        return;
    }
    start(owner, repo, false);
}

void AppPluginsGithub::start(const QString &owner,
                             const QString &repo,
                             bool update,
                             const QString &currentTag)
{
    if (_reply) {
        apxMsgW() << title().append(':') << tr("Busy");
        return;
    }
    _job = Job();
    _job.owner = owner;
    _job.repo = repo;
    _job.url = QString("https://github.com/%1/%2").arg(owner, repo);
    _job.update = update;
    _job.currentTag = currentTag;

    f_install->setEnabled(false);
    setProgress(0);
    apxMsg() << title().append(':') << tr("Checking") << _job.url;
    requestRelease();
}

QNetworkReply *AppPluginsGithub::get(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github+json, */*");
    request.setRawHeader("User-Agent",
                         QString("%1 (v%2)")
                             .arg(QCoreApplication::applicationName(),
                                  QCoreApplication::applicationVersion())
                             .toUtf8());
    _reply = _net.get(request);
    return _reply;
}

void AppPluginsGithub::requestRelease()
{
    auto reply = get(QUrl(QString("https://api.github.com/repos/%1/%2/releases/latest")
                              .arg(_job.owner, _job.repo)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (_reply != reply)
            return;
        _reply = nullptr;
        if (reply->error()) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            finish(false,
                   status == 404 ? tr("Repository or release not found").append(": ") + _job.url
                                 : reply->errorString());
            return;
        }
        QJsonParseError err;
        const auto json = QJsonDocument::fromJson(reply->readAll(), &err).object();
        _job.tag = json.value("tag_name").toString();
        if (err.error != QJsonParseError::NoError || _job.tag.isEmpty()) {
            finish(false, tr("Invalid release info"));
            return;
        }
        if (_job.update && _job.tag == _job.currentTag) {
            finish(true, tr("Up to date").append(": ") + _job.repo + " " + _job.tag);
            return;
        }
        const QString url = pickAsset(json.value("assets").toArray(), _job.assetName, _job.libName);
        if (url.isEmpty()) {
            finish(false,
                   tr("Release %1 has no plugin library for %2")
                       .arg(_job.tag, platformTags().join('/')));
            return;
        }
        // the plugin name is the library file name
        const QString pname = AppPlugins::pluginName(_job.libName);
        if (!_job.update && App::appPlugins()->plugin(pname)) {
            finish(false, tr("Plugin already exists").append(": ") + pname);
            return;
        }
        requestDownload(url);
    });
}

void AppPluginsGithub::requestDownload(const QString &url)
{
    QDir target(dir().absoluteFilePath(_job.repo));
    if (!target.mkpath(".")) {
        finish(false, tr("Can't create folder").append(": ") + target.absolutePath());
        return;
    }
    // download to <Name>.so.part, renamed when complete
    _file = new QFile(target.absoluteFilePath(_job.libName + ".part"), this);
    if (!_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        finish(false, tr("Can't write").append(": ") + _file->fileName());
        return;
    }
    apxMsg() << title().append(':') << tr("Downloading") << _job.assetName << _job.tag;
    auto reply = get(QUrl(url));
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 rcv, qint64 total) {
        if (_reply != reply || !_file)
            return;
        _file->write(reply->readAll());
        if (total > 0)
            setProgress(static_cast<int>(rcv * 100 / total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (_reply != reply)
            return;
        _reply = nullptr;
        if (!_file) {
            finish(false, tr("No data"));
            return;
        }
        if (reply->error()) {
            finish(false, reply->errorString());
            return;
        }
        _file->write(reply->readAll());
        _file->close();
        if (_file->size() <= 0) {
            finish(false, tr("No data"));
            return;
        }
        const QString libPath = _file->fileName().chopped(5); // .part
        QFile::remove(libPath);
        if (!_file->rename(libPath)) {
            finish(false, tr("Can't write").append(": ") + libPath);
            return;
        }
        _file->deleteLater();
        _file = nullptr;

        const QString pname = AppPlugins::pluginName(libPath);
        QJsonObject info;
        info["url"] = _job.url;
        info["owner"] = _job.owner;
        info["repo"] = _job.repo;
        info["tag"] = _job.tag;
        info["asset"] = _job.assetName;
        info["platform"] = platformTags().first();
        info["plugins"] = QJsonArray{pname};
        info["installed"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        if (!writeInfo(info))
            apxMsgW() << title().append(':') << tr("Can't write") << infoPath(_job.repo);

        if (_job.update) {
            apxMsgW() << title().append(':')
                      << tr("Restart the application to reload the updated plugin");
        } else {
            App::appPlugins()->loadGithubRepo(QFileInfo(libPath).absolutePath());
        }
        updateRepos();
        f_url->setValue(QString());
        finish(true,
               (_job.update ? tr("Updated") : tr("Installed")).append(": ") + pname + " " + _job.tag);
    });
}

void AppPluginsGithub::finish(bool ok, const QString &msg)
{
    _reply = nullptr;
    if (_file) {
        _file->close();
        _file->remove(); // incomplete download
        _file->deleteLater();
        _file = nullptr;
        // remove the folder created for the download when nothing else is there
        QDir target(dir().absoluteFilePath(_job.repo));
        if (!_job.update && target.exists() && target.isEmpty())
            target.removeRecursively();
    }
    setProgress(-1);
    f_install->setEnabled(true);
    if (msg.isEmpty())
        return;
    if (ok)
        apxMsg() << title().append(':') << msg;
    else
        apxMsgW() << title().append(':') << msg;
}

// installed repositories

bool AppPluginsGithub::writeInfo(const QJsonObject &info)
{
    QFile f(infoPath(info.value("repo").toString()));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(QJsonDocument(info).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject AppPluginsGithub::readInfo(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

void AppPluginsGithub::updateRepos()
{
    for (auto f : _repos.keys())
        f->deleteFact();
    _repos.clear();

    const QDir d = dir();
    if (!d.exists())
        return;
    for (const auto &repo :
         d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase)) {
        if (repo.startsWith('.') || repo.startsWith('-'))
            continue;
        const auto info = readInfo(infoPath(repo));
        if (info.value("repo").toString() != repo)
            continue;
        addRepoFact(info);
    }
}

Fact *AppPluginsGithub::addRepoFact(const QJsonObject &info)
{
    const QString repo = info.value("repo").toString();
    const QString tag = info.value("tag").toString();
    QStringList names;
    for (const auto &v : info.value("plugins").toArray())
        names.append(v.toString());

    Fact *f = new Fact(this,
                       repo.toLower(),
                       QString("%1/%2").arg(info.value("owner").toString(), repo),
                       info.value("url").toString(),
                       Group,
                       "source-repository");
    f->setSection(tr("Installed"));
    f->setValue(tag);
    _repos.insert(f, info);

    Fact *f_tag = new Fact(f, "release", tr("Release"), tr("Installed release tag"));
    f_tag->setValue(tag);
    Fact *f_plugins = new Fact(f, "plugins", tr("Plugins"), tr("Installed plugins"));
    f_plugins->setValue(names.join(", "));
    Fact *f_asset = new Fact(f, "asset", tr("Library"), tr("Installed release asset"));
    f_asset->setValue(info.value("asset").toString());

    Fact *f_update = new Fact(f,
                              "update",
                              tr("Update"),
                              tr("Download the latest release"),
                              Action | Apply,
                              "update");
    connect(f_update, &Fact::triggered, this, [this, f]() { updateRepo(f); });

    Fact *f_remove = new Fact(f,
                              "remove",
                              tr("Remove"),
                              tr("Delete the plugin folder"),
                              Action | Remove | CloseOnTrigger,
                              "delete");
    connect(f_remove, &Fact::triggered, this, [this, f]() { removeRepo(f); });
    return f;
}

void AppPluginsGithub::updateRepo(Fact *f)
{
    const auto info = _repos.value(f);
    start(info.value("owner").toString(),
          info.value("repo").toString(),
          true,
          info.value("tag").toString());
}

void AppPluginsGithub::removeRepo(Fact *f)
{
    const auto info = _repos.value(f);
    const QString repo = info.value("repo").toString();
    if (repo.isEmpty())
        return;
    const QString path = dir().absoluteFilePath(repo);

    auto plugins = App::appPlugins();
    bool restart = false;
    for (auto p : QList<AppPlugin *>(*plugins)) {
        if (!p->fileName.startsWith(path + "/"))
            continue;
        if (p->isLoaded()) {
            restart = true;
            continue;
        }
        plugins->removePlugin(p);
    }
    QDir(path).removeRecursively();
    apxMsg() << title().append(':') << tr("Removed") << repo;
    if (restart)
        apxMsgW() << title().append(':')
                  << tr("Restart the application to unload the removed plugin");
    updateRepos();
}
