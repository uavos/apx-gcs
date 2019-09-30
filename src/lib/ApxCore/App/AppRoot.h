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
#ifndef AppRoot_H
#define AppRoot_H
//=============================================================================
#include "AppPlugins.h"
#include "AppSettings.h"
#include <Fact/Fact.h>
//=============================================================================
class AppRoot : public Fact
{
    Q_OBJECT
public:
    explicit AppRoot(QObject *parent = nullptr);
    static AppRoot *instance() { return _instance; }

    AppSettings *f_settings;
    Fact *f_tools;
    Fact *f_controls;
    Fact *f_windows;
    Fact *f_pluginsSettings;

    void createTools();
    void addToolPlugin(AppPlugin *plugin);
    void addWindowPlugin(AppPlugin *plugin);
    void addControlPlugin(AppPlugin *plugin);

    //----------------------------------
    // static helpers and data converters
public:
    Q_INVOKABLE static QString latToString(double v);
    Q_INVOKABLE static QString lonToString(double v);
    Q_INVOKABLE static double latFromString(QString s);
    Q_INVOKABLE static double lonFromString(QString s);
    Q_INVOKABLE static QString distanceToString(uint v, bool units = true);
    Q_INVOKABLE static QString timeToString(quint64 v, bool seconds = false);
    Q_INVOKABLE static QString timemsToString(quint64 v);
    Q_INVOKABLE static quint64 timeFromString(QString s);

    Q_INVOKABLE static QString capacityToString(quint64 v);

    Q_INVOKABLE static double limit(double v, double min, double max);
    Q_INVOKABLE static double angle360(double v);
    Q_INVOKABLE static double angle90(double v);
    Q_INVOKABLE static double angle(double v);

    Q_INVOKABLE static QPointF rotate(const QPointF &p, double a);

    Q_INVOKABLE static QPointF seriesBounds(const QVariantList &series);

private:
    static AppRoot *_instance;

public:
    Q_INVOKABLE static void sound(const QString &v);

signals:
    void factTriggered(Fact *fact, QVariantMap opts);
};
//=============================================================================
#endif
