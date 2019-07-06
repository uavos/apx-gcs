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
#ifndef MandalaApp_H
#define MandalaApp_H
//=============================================================================
#include <QtCore>
#include <QJSEngine>
#include "MandalaTree.h"
#include "MandalaTreeVehicle.h"
#include "MandalaTreeField.h"
#include "node.h"
#include "AppProperties.h"
//=============================================================================
class MandalaApp : public MandalaTree
{
    Q_OBJECT
public:
    explicit MandalaApp(QObject *parent = 0);
    ~MandalaApp();

    MandalaTreeVehicle *addVehicle(IDENT::_ident *ident);

    AppProperties *prefs;

private:
    QJSEngine *js;
    QJSValue jsObj;
    QJSValue syncJS(QJSEngine *e, MandalaTree *m, QJSValue js_m);
    void syncJScurrent(QJSEngine *e, MandalaTree *m);

    //js helpers
    void add_js(QString fname, QString description, QString body);
    QHash<QString, QString> js_descr; //helper commands alias,descr

public:
    Q_INVOKABLE QJSValue jsexec(QString s);
    Q_INVOKABLE void help();
    Q_INVOKABLE void serial(uint8_t portID, QJSValue data);
    Q_INVOKABLE void can(uint32_t can_ID, uint8_t can_IDE, QJSValue data);
    Q_INVOKABLE void vmexec(QString func);
    Q_INVOKABLE void sleep(uint ms);

    //----------------------------------
    //extra math related methods
    Q_INVOKABLE static QString latToString(double v);
    Q_INVOKABLE static QString lonToString(double v);
    Q_INVOKABLE double static latFromString(QString s);
    Q_INVOKABLE double static lonFromString(QString s);
    Q_INVOKABLE static QString distanceToString(uint v);
    Q_INVOKABLE static QString timeToString(uint v);
    Q_INVOKABLE uint static timeFromString(QString s);

    Q_INVOKABLE static void toolTip(QString tooltip);
    Q_INVOKABLE static double limit(double v, double min, double max);
    Q_INVOKABLE static double angle360(double v);
    Q_INVOKABLE static double angle90(double v);
    Q_INVOKABLE static double angle(double v);

public slots:
    void downlinkReceived(const QByteArray &ba);
    void sound(QString text);

signals:
    void vehicleAdded(MandalaTree *v);
    void playSoundEffect(QString v);

    //PROPERTIES
public:
    Q_PROPERTY(MandalaTree *current READ current WRITE setCurrent NOTIFY neverChanged)
    MandalaTree *current();
    void setCurrent(MandalaTree *v);

    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)
    bool online();
    void updateOnline(bool v);

private:
    MandalaTree *m_current;
    MandalaTree *m_current_set;
signals:
    void neverChanged();
    void onlineChanged(bool);
public slots:
private:
    QTimer onlineTimer;
};
//Q_DECLARE_METATYPE(MandalaApp*)
//=============================================================================
#endif
