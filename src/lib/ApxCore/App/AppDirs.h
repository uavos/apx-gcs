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
#ifndef ApxDirs_H
#define ApxDirs_H
#include <QtCore>
//=============================================================================
class AppDirs
{
public:
    static QDir res();
    static QDir user();

    static QDir libs();

    static QDir plugins();
    static QDir userPlugins();

    static QDir firmware();

    static QDir prefs();
    static QDir missions();
    static QDir configs();
    static QDir scripts();
    static QDir db();
    static QDir logs();
    static QDir video();
    static QDir images();

    //-------------------------------------------
    //HELPERS
    static bool copyPath(QString sourceDir, QString destinationDir);
};
//=============================================================================
#endif
