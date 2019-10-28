#ifndef APPIMAGEAUTOUPDATER_H
#define APPIMAGEAUTOUPDATER_H

#include <QObject>
#include <QProgressDialog>
#include <QDialogButtonBox>
#include <memory>
#include <appimage/update.h>
#include "Fact/Fact.h"

class AppImageAutoUpdater: public Fact
{
    Q_OBJECT
    Q_PROPERTY(State state READ getState NOTIFY stateChanged)
    Q_PROPERTY(int updateProgress READ getUpdateProgress NOTIFY updateProgressChanged)
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
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

private:
    std::unique_ptr<appimage::update::Updater> m_updater;
    State m_state = NoUpdates;
    int m_updateProgress = 0;
    bool m_stopUpdate = false;

    void setState(State newState);
    void setUpdateProgress(int progress);

private slots:
    void onDialogYesClicked();

signals:
    void stateChanged();
    void updateProgressChanged();
};

#endif // APPIMAGEAUTOUPDATER_H
