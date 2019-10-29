#ifndef APPIMAGEAUTOUPDATER_H
#define APPIMAGEAUTOUPDATER_H

#include <QObject>
#include <QProgressDialog>
#include <QDialogButtonBox>
#include <memory>
#include "Fact/Fact.h"
#include <appimage/update.h>

class AppImageAutoUpdater: public Fact
{
    Q_OBJECT
    Q_PROPERTY(State state READ getState NOTIFY stateChanged)
    Q_PROPERTY(int updateProgress READ getUpdateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(QString statusMessage READ getStatusMessage NOTIFY statusMessageChanged)
public:
    enum State
    {
        CheckForUpdates,
        Updating,
        UpdateAvailable,
        NoUpdates
    };
    Q_ENUM(State)
    AppImageAutoUpdater(Fact *parent = nullptr);
    void checkForUpdates();
    void checkForUpdatesInBackground();
    void setAutomaticallyChecksForUpdates(bool b);
    State getState() const;
    int getUpdateProgress() const;
    QString getStatusMessage() const;
    Q_INVOKABLE void start(bool keepOldVersion);
    Q_INVOKABLE void stop();

private:
    State m_state = NoUpdates;
    int m_updateProgress = 0;
    bool m_stopUpdate = false;
    QString m_statusMessage;

    void setState(State newState);
    void setUpdateProgress(int progress);
    void setStatusMessage(const QString &status);

    std::shared_ptr<appimage::update::Updater> createUpdater(const QString &str, bool keepOldVersion);

signals:
    void stateChanged();
    void updateProgressChanged();
    void statusMessageChanged();
};

#endif // APPIMAGEAUTOUPDATER_H
