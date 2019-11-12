#ifndef APPIMAGEAUTOUPDATER_H
#define APPIMAGEAUTOUPDATER_H

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

#endif // APPIMAGEAUTOUPDATER_H
