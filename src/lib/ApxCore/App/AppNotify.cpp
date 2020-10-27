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
