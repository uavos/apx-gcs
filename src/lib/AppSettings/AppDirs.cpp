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
#include "AppDirs.h"
//=============================================================================
QDir AppDirs::plugins()
{
  return QDir(QCoreApplication::applicationDirPath()+"/../Plugins/gcs");
}
//=============================================================================
QDir AppDirs::res()
{
#ifdef __ANDROID__
  const QString hpath=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  return QDir("assets:/data/");
#else
  return QDir(QCoreApplication::applicationDirPath()+"/../Resources");
#endif
}
//=============================================================================
QDir AppDirs::user()
{
#ifdef __ANDROID__
  return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/.gcu");
#else
  return QDir(QDir(
    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    .absoluteFilePath("UAVOS") );
#endif
}
//=============================================================================
QDir AppDirs::telemetry()
{
  return QDir(user().absoluteFilePath("Telemetry"));
}
//=============================================================================
QDir AppDirs::maps()
{
  return QDir(user().absoluteFilePath("Maps"));
}
//=============================================================================
QDir AppDirs::lang()
{
  return QDir(QCoreApplication::applicationDirPath()+"/../Localization/gcs");
}
//=============================================================================
QDir AppDirs::missions()
{
  return QDir(user().absoluteFilePath("Missions"));
}
//=============================================================================
QDir AppDirs::configs()
{
  return QDir(user().absoluteFilePath("Configs"));
}
//=============================================================================
QDir AppDirs::nodes()
{
  return QDir(user().absoluteFilePath("Nodes"));
}
//=============================================================================
QDir AppDirs::scripts()
{
  return QDir(user().absoluteFilePath("Scripts"));
}
//=============================================================================
QDir AppDirs::userPlugins()
{
  return QDir(user().absoluteFilePath("Plugins"));
}
//=============================================================================
QDir AppDirs::db()
{
  return QDir(user().absoluteFilePath("Data"));
}
//=============================================================================
