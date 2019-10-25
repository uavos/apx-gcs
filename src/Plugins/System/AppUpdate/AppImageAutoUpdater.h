#ifndef APPIMAGEAUTOUPDATER_H
#define APPIMAGEAUTOUPDATER_H

#include <memory>
#include <appimage/update.h>

class AppImageAutoUpdater
{
public:
    AppImageAutoUpdater();
    void checkForUpdates();
    void checkForUpdatesInBackground();
    void setAutomaticallyChecksForUpdates(bool b);

private:
    std::unique_ptr<appimage::update::Updater> m_updater;
};

#endif // APPIMAGEAUTOUPDATER_H
