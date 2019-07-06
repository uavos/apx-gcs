#include "SparkleAutoUpdater.h"

#include <Cocoa/Cocoa.h>
#include <Sparkle/Sparkle.h>

class SparkleAutoUpdater::Private
{
public:
  SUUpdater* updater;
};

SparkleAutoUpdater::SparkleAutoUpdater()
{
  d = new Private;

  d->updater = [SUUpdater sharedUpdater];
  [d->updater retain];
}

SparkleAutoUpdater::~SparkleAutoUpdater()
{
  [d->updater release];
  delete d;
}

void SparkleAutoUpdater::checkForUpdatesInBackground()
{
  [d->updater checkForUpdatesInBackground];
}
void SparkleAutoUpdater::checkForUpdates()
{
  [d->updater checkForUpdates: 0];
}


void SparkleAutoUpdater::setFeedURL(const QString &v)
{
  NSURL* url = [NSURL URLWithString:
      [NSString stringWithUTF8String: v.toUtf8().data()]];
  [d->updater setFeedURL: url];
}
void SparkleAutoUpdater::setAutomaticallyChecksForUpdates(bool v)
{
  [d->updater setAutomaticallyChecksForUpdates: (BOOL)v];
}
void SparkleAutoUpdater::setAutomaticallyDownloadsUpdates(bool v)
{
  [d->updater setAutomaticallyDownloadsUpdates: (BOOL)v];
}
void SparkleAutoUpdater::setUpdateCheckInterval(double v)
{
  [d->updater setUpdateCheckInterval: (NSTimeInterval)v];
}
