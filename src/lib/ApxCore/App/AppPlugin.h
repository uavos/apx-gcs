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
#ifndef AppPlugin_H
#define AppPlugin_H
//=============================================================================
#include <App/PluginInterface.h>
#include <Fact/Fact.h>
#include <QtCore>
class AppPlugins;
//=============================================================================
class AppPlugin : public QObject
{
    Q_OBJECT
public:
    explicit AppPlugin(AppPlugins *plugins, QString name, QString fileName);
    ~AppPlugin();

    void load();
    void unload();

    AppPlugins *plugins;

    QString name;
    QString fileName;
    Fact *f_enabled;
    PluginInterface *interface;
    QObject *control;
    QPluginLoader *loader;
    QStringList depends;

    QString section;

    QString errorString();

private:
    QString m_errorString;

    void loadLib();
    void loadQml();

    bool checkLib(const QString &fname);

private slots:
    void enabledChanged();
};
//=============================================================================
#endif
