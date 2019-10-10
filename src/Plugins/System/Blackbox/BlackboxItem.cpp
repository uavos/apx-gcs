/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "BlackboxItem.h"
#include "BlackboxReader.h"

#include <App/AppLog.h>
//=============================================================================
BlackboxItem::BlackboxItem(Fact *parent,
                           const QString &name,
                           const QString &title,
                           const QString &descr,
                           Flags flags,
                           const QString &uid)
    : Fact(parent, name, title, descr, flags, "database")
    , reader(nullptr)
    , uid(uid)
    , totalSize(0)
{
    f_callsign = new Fact(this, "callsign", tr("Callsign"), tr("Vehicle identity"), Text);

    f_stats = new Fact(this, "stats", "", "", Const);
    connect(f_stats, &Fact::triggered, this, &BlackboxItem::getStats);

    //actions
    f_start = new Fact(this, "start", tr("Read"), tr("Download data"), Action | Apply, "download");
    connect(f_start, &Fact::triggered, this, &BlackboxItem::download);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop downloading"), Action | Stop, "stop");
    connect(f_stop, &Fact::triggered, this, &BlackboxItem::stop);
    connect(f_stop, &Fact::triggered, this, []() { apxMsgW() << tr("Blackbox download aborted"); });

    connect(this, &Fact::progressChanged, this, &BlackboxItem::updateActions);
    connect(this, &Fact::enabledChanged, this, &BlackboxItem::updateActions);

    updateActions();

    stop();
}
//=============================================================================
void BlackboxItem::updateActions()
{
    bool enb = enabled();
    int p = progress();
    f_start->setEnabled(enb && p < 0 && totalSize > 0);
    f_stop->setEnabled(p >= 0);

    f_stats->setProgress(p);
}
void BlackboxItem::updateStats()
{
    f_stats->setTitle(totalSize ? title().append(": ").append(parentFact()->title()) : tr("Empty"));
    f_stats->setStatus(QString("%1 MB").arg(totalSize / 1024.0 / 1024.0, 0, 'f', 2));
}
//=============================================================================
void BlackboxItem::getStats()
{
    f_stats->setTitle(tr("Getting blackbox info"));
}
void BlackboxItem::download()
{
    if (reader)
        return;
    apxMsg() << tr("Blackbox downloading").append("...");
    f_stats->setTitle(tr("Downloading").append("..."));

    //create tmp reader
    reader = new BlackboxReader(this, f_callsign->text().trimmed(), uid);
}

void BlackboxItem::stop()
{
    setProgress(-1);
    updateStats();
    if (reader) {
        reader->deleteLater();
        reader = nullptr;
    }
}
//=============================================================================
