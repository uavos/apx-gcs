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
#pragma once

#include "AppPlugins.h"
#include "AppSettings.h"
#include <Fact/Fact.h>

#include <QFont>
#include <QGeoCoordinate>

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

    //global progress
    void updateProgress(Fact *fact);

private:
    static AppRoot *_instance;
    QList<QPointer<Fact>> progressList;

    //----------------------------------
    // static helpers and data converters
public:
    Q_INVOKABLE static QString latToString(double v);
    Q_INVOKABLE static QString lonToString(double v);
    Q_INVOKABLE static double latFromString(QString s);
    Q_INVOKABLE static double lonFromString(QString s);
    Q_INVOKABLE static QString distanceToString(uint v, bool units = true);

    Q_INVOKABLE static QString timeToString(quint64 v, bool seconds = false); // value in [sec]
    Q_INVOKABLE static QString timeString(bool seconds = false);              // current time
    Q_INVOKABLE static QString dateToString(quint64 v);                       // seconds since epoch
    Q_INVOKABLE static QString timemsToString(quint64 v);                     // value in [ms]
    Q_INVOKABLE static quint64 timeFromString(QString s, bool seconds);       // returns [sec]

    Q_INVOKABLE static QString capacityToString(quint64 v, int prec = 0); // value in [bytes]

    Q_INVOKABLE static double limit(double v, double min, double max);
    Q_INVOKABLE static double angle360(double v);
    Q_INVOKABLE static double angle90(double v);
    Q_INVOKABLE static double angle(double v);

    Q_INVOKABLE static QPointF rotate(const QPointF &p, double a);

    Q_INVOKABLE static QPointF seriesBounds(const QVariantList &series);

    Q_INVOKABLE static QGeoCoordinate coordinate(double lat, double lon, double alt = 0);

    Q_INVOKABLE static QFont get_font(QString family,
                                      qreal size,
                                      bool bold = false,
                                      bool shaping = false);
    Q_INVOKABLE static QFont font_icons(qreal size);
    Q_INVOKABLE static QFont font(qreal size, bool bold = false);
    Q_INVOKABLE static QFont font_narrow(qreal size, bool bold = false);
    Q_INVOKABLE static QFont font_condenced(qreal size, bool bold = false);
    Q_INVOKABLE static QFont font_fixed(qreal size);

public:
    Q_INVOKABLE static void sound(const QString &v);

signals:
    void factTriggered(Fact *fact, QVariantMap opts);
};
