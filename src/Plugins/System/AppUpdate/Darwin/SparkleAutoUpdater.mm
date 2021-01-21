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
#include "SparkleAutoUpdater.h"

#include <Cocoa/Cocoa.h>
#include <Sparkle.h>

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
