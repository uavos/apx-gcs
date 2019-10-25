#include "AppImageAutoUpdater.h"

#include "App/AppDirs.h"
#include <QDebug>

AppImageAutoUpdater::AppImageAutoUpdater()
{
    std::string pathToAppImage = qgetenv("APPIMAGE").toStdString();
    if(!pathToAppImage.empty())
        m_updater = std::make_unique<appimage::update::Updater>(pathToAppImage, true);
}

void AppImageAutoUpdater::checkForUpdates()
{
    if(m_updater)
    {
        qDebug() << "checkForUpdates";
        bool updateAvailable = false;
        m_updater->checkForChanges(updateAvailable);
        qDebug() << updateAvailable;
    }
}

void AppImageAutoUpdater::checkForUpdatesInBackground()
{
    qDebug() << "checkForUpdatesInBackground";
}

void AppImageAutoUpdater::setAutomaticallyChecksForUpdates(bool b)
{
    qDebug() << "setAutomaticallyChecksForUpdates" << b;
}
