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
#include "AppImageAutoUpdater.h"

#include "App/App.h"
#include "App/AppDirs.h"
#include "App/AppLog.h"
#include <QMessageBox>
#include <QtQml>

AppImageAutoUpdater::AppImageAutoUpdater(Fact *parent)
    : Fact(parent, tr("appimage_updater"), tr("Good news everyone"))
{
    net.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    connect(
        this,
        &AppImageAutoUpdater::stateChanged,
        this,
        [this]() {
            if (getState() == UpdateAvailable)
                requestReleaseNotes();
        },
        Qt::QueuedConnection);

    //setup qml
    qmlRegisterType<AppImageAutoUpdater>("AppImageAutoUpdater", 1, 0, "AppImageAutoUpdater");
    setOpt("page", QString("qrc:/%1/AppImageAutoUpdater.qml").arg(PLUGIN_NAME));
    setVisible(false);
}

bool AppImageAutoUpdater::checkInstalled()
{
    if (App::installed())
        return true;
    apxMsgW() << tr("Application must be installed for updates to work");
    trigger();
    return false;
}

void AppImageAutoUpdater::checkForUpdates()
{
    checkInstalled();
    setState(CheckForUpdates);
    trigger();
    checkForUpdatesInBackground();
}

void AppImageAutoUpdater::checkForUpdatesInBackground()
{
    checkInstalled();
    auto updater = createUpdater(qgetenv("APPIMAGE"), false);
    if (updater) {
        auto t = std::thread([this, updater]() {
            bool updateAvailable = false;
            updater->checkForChanges(updateAvailable);
            if (updateAvailable) {
                setState(UpdateAvailable);
                trigger();
            } else
                setState(NoUpdates);
        });
        t.detach();
    }
}

void AppImageAutoUpdater::setAutomaticallyChecksForUpdates(bool b)
{
    Q_UNUSED(b)
}

AppImageAutoUpdater::State AppImageAutoUpdater::getState() const
{
    return m_state;
}

int AppImageAutoUpdater::getUpdateProgress() const
{
    return m_updateProgress;
}

QString AppImageAutoUpdater::getStatusMessage() const
{
    return m_statusMessage;
}

void AppImageAutoUpdater::start(bool keepOldVersion)
{
    checkInstalled();
    auto updater = createUpdater(qgetenv("APPIMAGE"), keepOldVersion);
    if (updater) {
        setUpdateProgress(0);
        setState(Updating);
        setStatusMessage("");
        m_stopUpdate = false;
        updater->start();
        while (!updater->isDone()) {
            if (updater->hasError()) {
                QMessageBox::critical(nullptr,
                                      QObject::tr("Error"),
                                      QObject::tr("Unknown error during update"),
                                      QMessageBox::Ok);
                setState(UpdateAvailable);
                return;
            }
            if (m_stopUpdate) {
                try {
                    updater->stop();
                } catch (std::exception &) {
                }
                setState(UpdateAvailable);
                return;
            }

            double progress = 0;
            updater->progress(progress);
            setUpdateProgress(qRound(progress * 100));
            QCoreApplication::processEvents();
            std::string nextMessage;
            while (updater->nextStatusMessage(nextMessage)) {
                setStatusMessage(QString::fromStdString(nextMessage));
                QCoreApplication::processEvents();
            }
        }

        //It's workaround for updater, that for some reason keep .zs-old file after update.
        QFile zsOld(qgetenv("APPIMAGE"));
        zsOld.setFileName(zsOld.fileName() + ".zs-old");
        if (zsOld.exists())
            zsOld.remove();

        std::string newFile;
        updater->pathToNewFile(newFile);
        QProcess::startDetached(QString::fromStdString(newFile));
        QCoreApplication::exit(0);

        setState(NoUpdates);
    }
}

void AppImageAutoUpdater::stop()
{
    m_stopUpdate = true;
}

void AppImageAutoUpdater::setState(State newState)
{
    if (m_state != newState) {
        m_state = newState;
        if (m_state == NoUpdates)
            setVisible(false);
        else
            setVisible(true);
        emit stateChanged();
    }
}

void AppImageAutoUpdater::setUpdateProgress(int progress)
{
    if (m_updateProgress != progress) {
        m_updateProgress = progress;
        emit updateProgressChanged();
    }
}

void AppImageAutoUpdater::setStatusMessage(const QString &status)
{
    if (m_statusMessage != status) {
        m_statusMessage = status;
        emit statusMessageChanged();
    }
}

std::shared_ptr<appimage::update::Updater> AppImageAutoUpdater::createUpdater(const QString &str,
                                                                              bool keepOldVersion)
{
    try {
        return std::make_shared<appimage::update::Updater>(str.toStdString(), !keepOldVersion);
    } catch (std::exception &e) {
        apxMsgW() << tr("Can't create AppImage updater instance: ") << e.what();
    }
    return std::shared_ptr<appimage::update::Updater>();
}

QNetworkReply *AppImageAutoUpdater::request(QUrl url)
{
    QNetworkRequest *request = new QNetworkRequest(url);

    QSslConfiguration ssl = request->sslConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    request->setSslConfiguration(ssl);

    request->setRawHeader("Accept", "*/*");
    request->setRawHeader("User-Agent",
                          QString("%1 (v%2)")
                              .arg(QCoreApplication::applicationName())
                              .arg(App::version())
                              .toUtf8());

    return net.get(*request);
}
void AppImageAutoUpdater::requestReleaseNotes()
{
    QUrl url("https://api.github.com/repos/uavos/apx-releases/releases/latest");
    QNetworkReply *reply = request(url);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error()) {
            qWarning() << reply->errorString();
            return;
        }
        QJsonDocument json(QJsonDocument::fromJson(reply->readAll()));
        QString sver = json["tag_name"].toString();
        qDebug() << "latest version:" << sver;
        if (sver.isEmpty())
            return;
        m_latestVersion = sver;
        emit latestVersionChanged();

        QUrl url(QString("https://uavos.github.io/apx-releases/notes/release-%1.md").arg(sver));
        QNetworkReply *reply2 = request(url);
        connect(reply2, &QNetworkReply::finished, this, [this, reply2]() {
            reply2->deleteLater();
            if (reply2->error()) {
                qWarning() << reply2->errorString();
                return;
            }
            m_releaseNotes = reply2->readAll();
            emit releaseNotesChanged();
            //qDebug() << m_releaseNotes;
        });
    });
}
