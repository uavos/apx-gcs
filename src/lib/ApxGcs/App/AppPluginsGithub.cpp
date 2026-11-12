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
           Group | ProgressTrack,
           "github")
{
    // install page: repository URL and the Install button
    f_install = new Fact(this,
                         "install",
                         tr("Install plugin"),
                         tr("Download a plugin released for version %1").arg(App::version()),
                         Group,
                         "download");

    f_url = new Fact(f_install,
                     "url",
                     tr("Repository"),
                     tr("GitHub repository URL (https://github.com/<owner>/<repo>)"),
                     Text);

    f_download = new Fact(f_install,
                          "download",
                          tr("Install"),
                          tr("Download the plugin library of the latest release for this version"),
                          Action | Apply | CloseOnTrigger,
                          "download");
    connect(f_download, &Fact::triggered, this, &AppPluginsGithub::install);

    // updates, like the app updates
    f_auto = new Fact(this,
                      "auto",
                      tr("Auto"),
                      tr("Install plugin updates automatically, otherwise show the updates list"),
                      Bool | PersistentValue);
    f_auto->setDefaultValue(false);
    f_auto->setSection(tr("Updates"));

    f_check = new Fact(this,
                       "checkupdates",
                       tr("Check for updates"),
                       tr("Find the latest plugin releases for version %1").arg(App::version()),
                       Action | Apply,
                       "update");
    f_check->setSection(tr("Updates"));
    connect(f_check, &Fact::triggered, this, &AppPluginsGithub::checkForUpdates);

    f_updates = new Fact(this,
                         "updates",
                         tr("Updates"),
                         tr("Plugin releases for version %1").arg(App::version()),
                         Group,
                         "download-multiple");
    f_updates->setSection(tr("Updates"));

    loadRepos();
    updateStatus();

    // check on start: auto installs the updates, otherwise the updates list is shown
    if (!_repos.isEmpty()) {
        QTimer::singleShot(5000, this, [this]() {
            // skip silently when something is already going on
            if (_checking > 0 || _installing)
                return;
            for (auto r : _repos) {
                if (r->busy())
                    return;
            }
            checkForUpdates();
        });
    }
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

AppPluginsGithubRepo *AppPluginsGithub::repo(const QString &name) const
{
    for (auto r : _repos) {
        if (r->repo().compare(name, Qt::CaseInsensitive) == 0)
            return r;
    }
    return nullptr;
}

// installed repositories: github/<repo>/<repo>.json

void AppPluginsGithub::loadRepos()
{
    const QDir d = dir();
    if (!d.exists())
        return;
    for (const auto &name :
         d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase)) {
        if (name.startsWith('.') || name.startsWith('-'))
            continue;
        const auto info = AppPlugins::githubRepoInfo(d.absoluteFilePath(name));
        if (info.value("repo").toString() != name)
            continue;
        addRepo(info);
    }
}

AppPluginsGithubRepo *AppPluginsGithub::addRepo(const QJsonObject &info)
{
    auto r = new AppPluginsGithubRepo(this, info);
    r->setSection(tr("Installed"));
    connectRepo(r);
    return r;
}

void AppPluginsGithub::connectRepo(AppPluginsGithubRepo *r)
{
    _repos.append(r);
    connect(r, &AppPluginsGithubRepo::checked, this, &AppPluginsGithub::repoChecked);
    connect(r, &AppPluginsGithubRepo::installed, this, &AppPluginsGithub::repoInstalled);
    connect(r, &AppPluginsGithubRepo::failed, this, &AppPluginsGithub::repoFailed);
    connect(r->f_remove, &Fact::triggered, this, [this, r]() { removeRepo(r); });
}

void AppPluginsGithub::deleteRepo(AppPluginsGithubRepo *r)
{
    _repos.removeAll(r);
    if (_installing == r)
        _installing = nullptr;
    r->deleteFact();
    updateStatus();
}

void AppPluginsGithub::removeRepo(AppPluginsGithubRepo *r)
{
    if (r->busy())
        r->abort();
    const QString path = r->dir().absolutePath();

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
    apxMsg() << title().append(':') << tr("Removed") << r->repo();
    if (restart)
        apxMsgW() << title().append(':')
                  << tr("Restart the application to unload the removed plugin");
    deleteRepo(r);
}

// install

void AppPluginsGithub::install()
{
    QString owner, name;
    if (!parseRepoUrl(f_url->text(), owner, name)) {
        apxMsgW() << title().append(':') << tr("Invalid repository URL") << f_url->text();
        return;
    }
    if (repo(name) || QFileInfo::exists(dir().absoluteFilePath(name))) {
        apxMsgW() << title().append(':') << tr("Already installed") << name;
        return;
    }
    if (_installing) {
        apxMsgW() << title().append(':') << tr("Busy");
        return;
    }
    auto r = new AppPluginsGithubRepo(this, owner, name);
    r->setSection(tr("Installing"));
    r->f_update->setVisible(false);
    r->f_remove->setVisible(false);
    connectRepo(r);
    _installing = r;
    f_download->setEnabled(false);
    apxMsg() << title().append(':') << tr("Checking") << r->url();
    r->check();
}

void AppPluginsGithub::repoChecked()
{
    auto r = qobject_cast<AppPluginsGithubRepo *>(sender());
    if (!r)
        return;

    if (r == _installing) {
        // the release for this version is found: check the plugin name and download
        if (!r->hasUpdate()) {
            apxMsgW() << title().append(':') << r->status();
            deleteRepo(r);
            return;
        }
        const QString pname = r->latestPlugin();
        if (App::appPlugins()->plugin(pname)) {
            apxMsgW() << title().append(':') << tr("Plugin already exists").append(": ") << pname;
            deleteRepo(r);
            return;
        }
        r->install();
        return;
    }

    updateStatus();

    if (_checking == 0)
        return;
    _checking--;

    // auto: install the release right away
    if (f_auto->value().toBool()) {
        if (r->hasUpdate()) {
            apxMsg() << title().append(':') << tr("Updating") << r->title() << r->latestTag();
            r->install();
        }
        return;
    }
    // show the updates list when all repositories are checked
    if (_checking == 0 && f_updates->enabled())
        f_updates->trigger();
}

void AppPluginsGithub::repoInstalled(QString libPath)
{
    auto r = qobject_cast<AppPluginsGithubRepo *>(sender());
    if (!r)
        return;

    if (r == _installing) {
        _installing = nullptr;
        f_download->setEnabled(true);
        f_url->setValue(QString());
        r->setSection(tr("Installed"));
        r->f_update->setVisible(true);
        r->f_remove->setVisible(true);
    }

    // load the plugin when it is not loaded yet (new or disabled by version mismatch)
    auto plugins = App::appPlugins();
    auto p = plugins->plugin(AppPlugins::pluginName(libPath));
    if (!p) {
        plugins->loadGithubRepo(QFileInfo(libPath).absolutePath());
    } else if (p->isLoaded()) {
        apxMsgW() << title().append(':')
                  << tr("Restart the application to reload the updated plugin");
    }
    updateStatus();
}

void AppPluginsGithub::repoFailed()
{
    auto r = qobject_cast<AppPluginsGithubRepo *>(sender());
    if (!r)
        return;
    if (r == _installing) {
        f_download->setEnabled(true);
        // remove the folder created for the download when nothing else is there
        QDir target(r->dir());
        if (target.exists() && target.isEmpty())
            target.removeRecursively();
        deleteRepo(r);
        return;
    }
    if (_checking > 0)
        _checking--;
    updateStatus();
}

// updates

void AppPluginsGithub::checkForUpdates()
{
    if (_checking > 0) {
        apxMsgW() << title().append(':') << tr("Busy");
        return;
    }
    QList<AppPluginsGithubRepo *> list;
    int installed = 0;
    for (auto r : _repos) {
        if (!r->isInstalled())
            continue;
        installed++;
        if (!r->busy())
            list.append(r);
    }
    if (installed == 0) {
        apxMsg() << title().append(':') << tr("No plugins installed");
        return;
    }
    if (list.isEmpty()) {
        apxMsgW() << title().append(':') << tr("Busy");
        return;
    }
    apxMsg() << title().append(':') << tr("Checking for updates") << App::version();
    _checking = list.size();
    for (auto r : list)
        r->check();
}

// the updates list: a plugin per entry, enabled when a release for this version can be installed
void AppPluginsGithub::updateStatus()
{
    f_updates->deleteChildren();

    int checked = 0, updates = 0;
    for (auto r : _repos) {
        if (!r->isInstalled() || !r->isChecked())
            continue;
        checked++;
        // up to date
        if (!r->hasUpdate() && r->isCompatible())
            continue;
        // plain item: click installs the release
        Fact *f = new Fact(f_updates,
                           r->name(),
                           r->title(),
                           r->status(),
                           NoFlags,
                           r->hasUpdate() ? "download" : "alert-circle-outline");
        if (r->hasUpdate()) {
            updates++;
            f->setValue(r->latestTag());
            connect(f, &Fact::triggered, r, &AppPluginsGithubRepo::install);
        } else {
            f->setEnabled(false);
        }
    }

    f_updates->setEnabled(f_updates->size() > 0);
    if (checked == 0)
        f_updates->setTitle(tr("Updates"));
    else if (updates > 0)
        f_updates->setTitle(tr("Updates available: %1").arg(updates));
    else
        f_updates->setTitle(tr("No updates available"));
    f_updates->setValue(f_updates->size() > 0 ? QString::number(f_updates->size()) : QString());
}
