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
#ifndef AppBase_H
#define AppBase_H
#include <QApplication>
//=============================================================================
class AppBase : public QApplication
{
    Q_OBJECT

public:
    explicit AppBase(int &argc, char **argv, const QString &name);

    //constants
    Q_INVOKABLE static QString version() { return _instance->m_version; }
    Q_INVOKABLE static QString branch() { return _instance->m_branch; }
    Q_INVOKABLE static QString git_hash() { return _instance->m_hash; }
    Q_INVOKABLE static QString git_time() { return _instance->m_time; }
    Q_INVOKABLE static QString git_year() { return _instance->m_year; }

    Q_INVOKABLE static QString aboutString();
    Q_INVOKABLE static QString compilerString();

    Q_INVOKABLE static QString machineUID() { return _instance->m_machineUID; }
    Q_INVOKABLE static QString hostname() { return _instance->m_hostname; }
    Q_INVOKABLE static QString username() { return _instance->m_username; }

    Q_INVOKABLE static bool dryRun() { return _instance->m_dryRun; }
    Q_INVOKABLE static bool segfault() { return _instance->m_segfault; }

    Q_INVOKABLE static bool installed() { return _instance->m_installed; }
    Q_INVOKABLE static QString installDir() { return _instance->m_installDir; }
    Q_INVOKABLE static QString bundlePath() { return _instance->m_bundlePath; }
    Q_INVOKABLE static bool install();

private:
    static AppBase *_instance;

    bool m_dryRun;
    bool m_segfault;

    QString m_version;
    QString m_branch;
    QString m_hash;
    QString m_time;
    QString m_year;
    QString m_machineUID;
    QString m_hostname;
    QString m_username;

    bool m_installed;
    QString m_installDir;
    QString m_bundlePath;
};
//=============================================================================
#endif
