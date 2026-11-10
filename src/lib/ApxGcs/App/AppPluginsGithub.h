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

#include <Fact/Fact.h>
#include <QtCore>
#include <QtNetwork>

/*
 * Third party QML plugins installed from GitHub repositories.
 * Sources are kept in AppDirs::githubPlugins()/<repo>, the release info in <repo>.json next to it.
 * The plugins (<Name>Plugin.qml files of the repository) are loaded by AppPlugins
 * like the other user plugins and are switched on and off in the plugins settings.
 */
class AppPluginsGithub : public Fact
{
    Q_OBJECT
public:
    explicit AppPluginsGithub(Fact *parent);

    // https://github.com/<owner>/<repo>[.git], github.com/<owner>/<repo>, <owner>/<repo>
    static bool parseRepoUrl(const QString &url, QString &owner, QString &repo);

private:
    Fact *f_url;
    Fact *f_install;

    QNetworkAccessManager _net;
    QNetworkReply *_reply{};
    QTemporaryFile *_file{};

    struct Job
    {
        QString owner;
        QString repo;
        QString url;
        QString tag;
        QString name;
        bool update{};
        QString currentTag;
    } _job;

    QHash<Fact *, QJsonObject> _repos;

    QNetworkReply *get(const QUrl &url);
    void start(const QString &owner, const QString &repo, bool update, const QString &currentTag = {});
    void requestRelease();
    void requestZipball(const QString &url);
    void extract();
    void finish(bool ok, const QString &msg = {});

    static QDir dir();
    static QString infoPath(const QString &repo);
    static bool writeInfo(const QJsonObject &info);
    static QJsonObject readInfo(const QString &path);

    void updateRepos();
    Fact *addRepoFact(const QJsonObject &info);
    void updateRepo(Fact *f);
    void removeRepo(Fact *f);

private slots:
    void install();
};
