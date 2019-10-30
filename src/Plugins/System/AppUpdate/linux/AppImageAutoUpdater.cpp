#include "AppImageAutoUpdater.h"

#include "App/AppDirs.h"
#include "App/AppLog.h"
#include <QMessageBox>
#include <QtQml>

AppImageAutoUpdater::AppImageAutoUpdater(Fact *parent)
    : Fact(parent, tr("appimage_updater"), tr("Good news everyone"))
{
    qmlRegisterType<AppImageAutoUpdater>("AppImageAutoUpdater", 1, 0, "AppImageAutoUpdater");
    setQmlPage(QString("qrc:/%1/AppImageAutoUpdater.qml").arg(PLUGIN_NAME));
    setVisible(false);
}

void AppImageAutoUpdater::checkForUpdates()
{
    setState(CheckForUpdates);
    trigger();
    checkForUpdatesInBackground();
}

void AppImageAutoUpdater::checkForUpdatesInBackground()
{
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
