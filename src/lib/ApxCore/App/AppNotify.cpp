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
#include "AppNotify.h"
#include "AppLog.h"
#include <Fact/Fact.h>

AppNotify *AppNotify::_instance = nullptr;
//=============================================================================
AppNotify::AppNotify(QObject *parent)
    : QObject(parent)
{
    _instance = this;

    connect(AppLog::instance(),
            &AppLog::infoMessage,
            this,
            &AppNotify::logInfoMessage,
            Qt::QueuedConnection);
    connect(AppLog::instance(),
            &AppLog::warningMessage,
            this,
            &AppNotify::logWarningMessage,
            Qt::QueuedConnection);
}
//=============================================================================
//=============================================================================
void AppNotify::report(QString msg, AppNotify::NotifyFlags flags, QString subsystem)
{
    emit notification(msg, subsystem, flags, nullptr);
}
void AppNotify::report(Fact *fact)
{
    emit notification("", "", FromApp, fact);
}
void AppNotify::logInfoMessage(QString msg)
{
    emit notification(msg, QString(), Console, nullptr);
}
void AppNotify::logWarningMessage(QString msg)
{
    emit notification(msg, QString(), Console | Warning, nullptr);
}
//=============================================================================
//=============================================================================
