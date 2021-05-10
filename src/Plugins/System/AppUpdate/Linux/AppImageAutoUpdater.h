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

#include "Fact/Fact.h"
#include <appimage/update.h>
#include <memory>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class AppImageAutoUpdater : public Fact
{
    Q_OBJECT
    Q_PROPERTY(State state READ getState NOTIFY stateChanged)
    Q_PROPERTY(int updateProgress READ getUpdateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(QString statusMessage READ getStatusMessage NOTIFY statusMessageChanged)

    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY latestVersionChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY releaseNotesChanged)
public:
    enum State { CheckForUpdates, Updating, UpdateAvailable, NoUpdates };
    Q_ENUM(State)
    AppImageAutoUpdater(Fact *parent = nullptr);
    void checkForUpdates();
    void checkForUpdatesInBackground();
    void setAutomaticallyChecksForUpdates(bool b);
    State getState() const;
    int getUpdateProgress() const;
    QString getStatusMessage() const;
    Q_INVOKABLE void start(bool keepOldVersion = false);
    Q_INVOKABLE void stop();

    bool checkInstalled();

private:
    State m_state = NoUpdates;
    int m_updateProgress = 0;
    bool m_stopUpdate = false;
    QString m_statusMessage;

    void setState(State newState);
    void setUpdateProgress(int progress);
    void setStatusMessage(const QString &status);

    std::shared_ptr<appimage::update::Updater> createUpdater(const QString &str,
                                                             bool keepOldVersion);

    QNetworkAccessManager net;
    void requestReleaseNotes();
    QNetworkReply *request(QUrl url);
    QString m_latestVersion;
    QString latestVersion() const { return m_latestVersion; }
    QString m_releaseNotes;
    QString releaseNotes() const { return m_releaseNotes; }

signals:
    void stateChanged();
    void updateProgressChanged();
    void statusMessageChanged();
    void latestVersionChanged();
    void releaseNotesChanged();
};
