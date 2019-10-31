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
#ifndef AppNotify_H
#define AppNotify_H
//=============================================================================
#include <Fact/Fact.h>
//=============================================================================
class AppNotify : public QObject
{
    Q_OBJECT

public:
    explicit AppNotify(QObject *parent = nullptr);

    static AppNotify *instance() { return _instance; }

    enum NotifyFlag {
        //message origin [enum]
        NotifySourceMask = 0x0F,
        FromApp = 0,     // msg from app
        FromInput = 1,   // msg from user terminal input
        FromVehicle = 2, // msg from vehicle

        //importance [enum]
        NotifyTypeMask = 0xF0,
        Info = 0 << 4,
        Important = 1 << 4,
        Warning = 2 << 4,
        Error = 3 << 4,

        //options [bits]
        NotifyOptionsMask = 0xF00,
        Console = 1 << 8, // msg from log stream
    };
    Q_DECLARE_FLAGS(NotifyFlags, NotifyFlag)
    Q_FLAG(NotifyFlags)
    Q_ENUM(NotifyFlag)

private:
    static AppNotify *_instance;

public slots:
    void report(QString msg,
                AppNotify::NotifyFlags flags = AppNotify::NotifyFlags(FromApp),
                QString subsystem = QString());
    void report(Fact *fact);
private slots:
    void logInfoMessage(QString msg);
    void logWarningMessage(QString msg);

signals:
    void notification(QString msg, QString subsystem, AppNotify::NotifyFlags flags, Fact *fact);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AppNotify::NotifyFlags)
//=============================================================================
#endif
