#include "AppImageAutoUpdater.h"

#include "App/AppDirs.h"
#include <QtQml>
#include <QMessageBox>
#include <QDebug>
#include <QObject>
#include <QProgressDialog>
#include <future>
#include <QDialogButtonBox>

AppImageAutoUpdater::AppImageAutoUpdater(Fact *parent):
    Fact(parent, tr("appimage_updater"), tr("Good news everyone"))
{
    qmlRegisterType<AppImageAutoUpdater>("AppImageAutoUpdater", 1, 0, "AppImageAutoUpdater");
    std::string pathToAppImage = qgetenv("APPIMAGE").toStdString();
    if(!pathToAppImage.empty())
    {
        m_updater = std::make_unique<appimage::update::Updater>(pathToAppImage, true);
    }

    setQmlPage(QString("qrc:/%1/AppImageAutoUpdater.qml").arg(PLUGIN_NAME));
}

void AppImageAutoUpdater::checkForUpdates()
{
    if(m_updater)
    {
        setState(CheckForUpdates);
        trigger();
        checkForUpdatesInBackground();
    }
}

void AppImageAutoUpdater::checkForUpdatesInBackground()
{
    if(m_updater)
    {
        auto t = std::thread([this](){
            bool updateAvailable = false;
            m_updater->checkForChanges(updateAvailable);
            if(updateAvailable)
            {
                setState(UpdateAvailable);
                trigger();
            }
            else
                setState(NoUpdates);
        });
        t.detach();
    }
}

void AppImageAutoUpdater::setAutomaticallyChecksForUpdates(bool b)
{
    qDebug() << "setAutomaticallyChecksForUpdates" << b;
}

AppImageAutoUpdater::State AppImageAutoUpdater::getState() const
{
    return m_state;
}

int AppImageAutoUpdater::getUpdateProgress() const
{
    return m_updateProgress;
}

void AppImageAutoUpdater::start()
{
    if(m_updater)
    {
        setUpdateProgress(0);
        setState(Updating);
        m_stopUpdate = false;
        m_updater->start();
        while(!m_updater->isDone())
        {
            if(m_updater->hasError())
            {
                QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Unknown error during update"),
                                      QMessageBox::Ok);
                setState(UpdateAvailable);
            }
            if(m_stopUpdate)
            {
                try
                {
                    m_updater->stop();
                }
                catch(std::exception&)
                {
                }
                setState(UpdateAvailable);
            }

            double progress = 0;
            m_updater->progress(progress);
            setUpdateProgress(qRound(progress * 100));
            QCoreApplication::processEvents();
        }
        setState(NoUpdates);
    }
}

void AppImageAutoUpdater::stop()
{
    m_stopUpdate = true;
}

void AppImageAutoUpdater::setState(State newState)
{
    if(m_state != newState)
    {
        m_state = newState;
        if(m_state == NoUpdates)
            setEnabled(false);
        else
            setEnabled(true);
        emit stateChanged();
    }
}

void AppImageAutoUpdater::setUpdateProgress(int progress)
{
    if(m_updateProgress != progress)
    {
        m_updateProgress = progress;
        emit updateProgressChanged();
    }
}
