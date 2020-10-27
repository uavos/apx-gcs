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
