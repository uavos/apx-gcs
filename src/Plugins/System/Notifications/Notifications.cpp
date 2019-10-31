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
#include "Notifications.h"
#include "NotifyItem.h"
#include <App/App.h>
#include <App/AppRoot.h>
//=============================================================================
Notifications::Notifications(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Notifications"),
           tr("Application notifications"),
           Group,
           "comment")
    , notifyEvent(100, true)
{
    connect(&notifyEvent, &DelayedEvent::triggered, this, &Notifications::updateItems);

    connect(AppNotify::instance(), &AppNotify::notification, this, &Notifications::appNotification);

    loadQml("qrc:/" PLUGIN_NAME "/NotificationsPlugin.qml");
}
//=============================================================================
void Notifications::appNotification(QString msg,
                                    QString subsystem,
                                    AppNotify::NotifyFlags flags,
                                    Fact *fact)
{
    if (items.contains(fact))
        return;
    if (newItems.contains(fact))
        return;
    newItems.append(fact);
    notifyEvent.schedule();
}
//=============================================================================
//=============================================================================
void Notifications::updateItems()
{
    for (int i = 0; i < newItems.size(); ++i) {
        Fact *fact = newItems.at(i);
        if (!fact)
            continue;
        //check parents
        bool found = false;
        for (Fact *f = fact->parentFact(); f; f = f->parentFact()) {
            if (newItems.contains(f) || items.contains(f)) {
                found = true;
                break;
            }
        }
        if (!found)
            createItem(fact);
    }
    newItems.clear();
}
//=============================================================================
void Notifications::createItem(Fact *fact)
{
    NotifyItem *f = items.value(fact);
    if (f)
        return;
    //create new item
    f = new NotifyItem(fact, this);
    items.insert(fact, f);
}
//=============================================================================
//=============================================================================
//=============================================================================
