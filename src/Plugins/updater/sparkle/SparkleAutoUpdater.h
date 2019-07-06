#ifndef SPARKLEAUTOUPDATER_H
#define SPARKLEAUTOUPDATER_H

#include <QString>

class SparkleAutoUpdater
{
public:
    SparkleAutoUpdater();
    ~SparkleAutoUpdater();

    void checkForUpdatesInBackground();
    void checkForUpdates();

    void setFeedURL(const QString &v);
    void setAutomaticallyChecksForUpdates(bool v);
    void setAutomaticallyDownloadsUpdates(bool v);
    void setUpdateCheckInterval(double v);

private:
    class Private;
    Private *d;
};

#endif
