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

#include <quazip/JlCompress.h>

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
                         tr("Download the latest release and add its plugins"),
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

QDir AppPluginsGithub::dir()
{
    return AppDirs::githubPlugins();
}

QString AppPluginsGithub::infoPath(const QString &repo)
{
    return dir().absoluteFilePath(repo + ".json");
}

// install

void AppPluginsGithub::install()
{
    QString owner, repo;
    if (!parseRepoUrl(f_url->text(), owner, repo)) {
        apxMsgW() << title().append(':') << tr("Invalid repository URL") << f_url->text();
        return;
    }
    if (QFileInfo::exists(dir().absoluteFilePath(repo)) || QFileInfo::exists(infoPath(repo))) {
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
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) {
            // no releases: take the default branch
            _job.tag = "HEAD";
            requestZipball(QString("https://api.github.com/repos/%1/%2/zipball")
                               .arg(_job.owner, _job.repo));
            return;
        }
        if (reply->error()) {
            finish(false, reply->errorString());
            return;
        }
        QJsonParseError err;
        const auto json = QJsonDocument::fromJson(reply->readAll(), &err).object();
        _job.tag = json.value("tag_name").toString();
        if (err.error != QJsonParseError::NoError || _job.tag.isEmpty()) {
            finish(false, tr("Invalid release info"));
            return;
        }
        _job.name = json.value("name").toString();
        if (_job.update && _job.tag == _job.currentTag) {
            finish(true, tr("Up to date").append(": ") + _job.repo + " " + _job.tag);
            return;
        }
        QString zip = json.value("zipball_url").toString();
        if (zip.isEmpty())
            zip = QString("https://api.github.com/repos/%1/%2/zipball/%3")
                      .arg(_job.owner, _job.repo, _job.tag);
        requestZipball(zip);
    });
}

void AppPluginsGithub::requestZipball(const QString &url)
{
    _file = new QTemporaryFile(this);
    if (!_file->open()) {
        finish(false, tr("Can't create temporary file"));
        return;
    }
    apxMsg() << title().append(':') << tr("Downloading") << _job.repo << _job.tag;
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
        if (reply->error()) {
            finish(false, reply->errorString());
            return;
        }
        if (!_file) {
            finish(false, tr("No data"));
            return;
        }
        _file->write(reply->readAll());
        _file->flush();
        if (_file->size() <= 0) {
            finish(false, tr("No data"));
            return;
        }
        extract();
    });
}

void AppPluginsGithub::extract()
{
    auto plugins = App::appPlugins();

    QDir root = dir();
    root.mkpath(".");
    QDir tmp(root.absoluteFilePath(".tmp-" + _job.repo));
    tmp.removeRecursively();
    tmp.mkpath(".");

    _file->seek(0);
    const auto files = JlCompress::extractDir(_file, tmp.absolutePath());
    _file->deleteLater();
    _file = nullptr;
    if (files.isEmpty()) {
        tmp.removeRecursively();
        finish(false, tr("Can't extract archive"));
        return;
    }

    // the archive contains a single top level folder <owner>-<repo>-<commit>
    const auto tops = tmp.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    const QString src = tops.size() == 1 ? tmp.absoluteFilePath(tops.first()) : tmp.absolutePath();

    const auto qmlFiles = AppPlugins::findQmlPlugins(QDir(src));
    if (qmlFiles.isEmpty()) {
        tmp.removeRecursively();
        finish(false, tr("No plugins found in repository").append(": ") + _job.url);
        return;
    }
    QStringList names;
    for (const auto &f : qmlFiles)
        names.append(AppPlugins::pluginName(f));

    const QString target = root.absoluteFilePath(_job.repo);
    if (_job.update) {
        QDir(target).removeRecursively();
    } else {
        for (const auto &n : names) {
            if (!plugins->plugin(n))
                continue;
            tmp.removeRecursively();
            finish(false, tr("Plugin already exists").append(": ") + n);
            return;
        }
        if (QFileInfo::exists(target)) {
            tmp.removeRecursively();
            finish(false, tr("Folder already exists").append(": ") + target);
            return;
        }
    }
    if (!QDir().rename(src, target)) {
        tmp.removeRecursively();
        finish(false, tr("Can't move files to").append(" ") + target);
        return;
    }
    tmp.removeRecursively();

    QJsonObject info;
    info["url"] = _job.url;
    info["owner"] = _job.owner;
    info["repo"] = _job.repo;
    info["tag"] = _job.tag;
    info["name"] = _job.name;
    info["plugins"] = QJsonArray::fromStringList(names);
    info["installed"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!writeInfo(info)) {
        apxMsgW() << title().append(':') << tr("Can't write") << infoPath(_job.repo);
    }

    if (_job.update) {
        apxMsgW() << title().append(':')
                  << tr("Restart the application to reload the updated plugins");
    } else {
        plugins->loadGithubRepo(target);
    }
    updateRepos();
    f_url->setValue(QString());
    finish(true,
           (_job.update ? tr("Updated") : tr("Installed")).append(": ") + _job.repo + " " + _job.tag
               + " (" + names.join(", ") + ")");
}

void AppPluginsGithub::finish(bool ok, const QString &msg)
{
    _reply = nullptr;
    if (_file) {
        _file->deleteLater();
        _file = nullptr;
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
    for (const auto &fi : d.entryInfoList({"*.json"}, QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        const auto info = readInfo(fi.absoluteFilePath());
        if (info.value("repo").toString() != fi.completeBaseName())
            continue;
        if (!QFileInfo::exists(d.absoluteFilePath(fi.completeBaseName())))
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
    Fact *f_plugins = new Fact(f, "plugins", tr("Plugins"), tr("Plugins found in the repository"));
    f_plugins->setValue(names.join(", "));

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
                              tr("Delete the repository folder and its plugins"),
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
    QFile::remove(infoPath(repo));
    apxMsg() << title().append(':') << tr("Removed") << repo;
    if (restart)
        apxMsgW() << title().append(':')
                  << tr("Restart the application to unload the removed plugins");
    updateRepos();
}
