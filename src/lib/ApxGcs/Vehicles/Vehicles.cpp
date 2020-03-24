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
#include "Vehicles.h"
#include "VehicleSelect.h"
#include "VehicleWarnings.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <Database/TelemetryDB.h>
#include <QQmlEngine>

APX_LOGGING_CATEGORY(VehiclesLog, "core.vehicles")
Vehicles *Vehicles::_instance = nullptr;

Vehicles::Vehicles(Fact *parent, ProtocolVehicles *protocol)
    : ProtocolViewBase(parent, protocol)
{
    _instance = this;

    setName("vehicles");
    setTitle(tr("Vehicles"));
    setDescr(tr("Discovered vehicles"));
    setOption(Section);

    qmlRegisterUncreatableType<Vehicles>("APX.Vehicles", 1, 0, "Vehicles", "Reference only");
    qmlRegisterUncreatableType<Vehicle>("APX.Vehicles", 1, 0, "Vehicle", "Reference only");
    qmlRegisterUncreatableType<VehicleWarnings>("APX.Vehicles",
                                                1,
                                                0,
                                                "VehicleWarnings",
                                                "Reference only");

    qmlRegisterUncreatableType<ProtocolTraceItem>("APX.Protocols",
                                                  1,
                                                  0,
                                                  "Protocols",
                                                  "Reference only");

    f_select = new VehicleSelect(this,
                                 "select",
                                 tr("Select vehicle"),
                                 tr("Change the active vehicle"));
    f_select->setIcon("select");
    f_select->setTreeType(Action);
    connect(f_select, &VehicleSelect::vehicleSelected, this, &Vehicles::selectVehicle);

    f_local = new Vehicle(this, protocol->local);
    f_select->addVehicle(f_local);

    f_replay = new Vehicle(this, protocol->replay);
    f_select->addVehicle(f_replay);

    //JS register mandala
    if (App::instance()->engine()) {
        App::jsync(this);

        //register mandala constants for QML and JS
        for (auto s : f_local->f_mandala->constants.keys()) {
            const QVariant &v = f_local->f_mandala->constants.value(s);
            //JSEngine layer
            App::setGlobalProperty(s, v);
            //QmlEngine layer
            App::setContextProperty(s, v);
        }

        jsSyncMandalaAccess(f_local->f_mandala, App::instance()->engine()->globalObject());
    }

    //Database register fields
    DatabaseRequest::Records recMandala;
    recMandala.names << "id"
                     << "name"
                     << "title"
                     << "descr"
                     << "units"
                     << "alias";
    foreach (MandalaFact *f, f_local->f_mandala->uid_map.values()) {
        if (f->isSystem())
            continue;
        QVariantList v;
        v << f->offset() << f->mpath() << f->title() << f->descr();
        v << (f->enumStrings().isEmpty() ? f->units() : f->enumStrings().join(','));
        v << f->alias();
        recMandala.values.append(v);
    }
    DBReqTelemetryUpdateMandala *req = new DBReqTelemetryUpdateMandala(recMandala);
    connect(
        req,
        &DBReqTelemetryUpdateMandala::progress,
        this,
        [](int v) {
            AppRoot::instance()->setValue(
                v < 0 ? QVariant()
                      : tr("Telemetry DB maintenance - stand by").toUpper().append("..."));
        },
        Qt::QueuedConnection);
    req->exec();

    selectVehicle(f_local);

    f_local->dbSaveVehicleInfo();

    //connect protocols
    connect(protocol, &ProtocolVehicles::vehicleIdentified, this, &Vehicles::vehicleIdentified);
}

void Vehicles::vehicleIdentified(ProtocolVehicle *protocol)
{
    Vehicle *v = new Vehicle(this, protocol);

    emit vehicleRegistered(v);

    QString msg = QString("%1: %2").arg(tr("Vehicle identified")).arg(v->title());
    if (protocol->squawk() > 0)
        msg.append(QString(" (%1)").arg(protocol->squawkText()));
    v->message(msg, AppNotify::Important);

    v->dbSaveVehicleInfo();

    /* FIXME: select identified vehicle
     while (f_list->size() > 0) {
        if (m_current->vehicleClass() == Vehicle::UAV)
            break;
        selectVehicle(v);
        break;
    }*/
}

void Vehicles::selectVehicle(Vehicle *v)
{
    if (!v)
        return;

    if (m_current) {
        m_current->setActive(false);
    }

    v->setActive(true); //ensure is active

    if (m_current == v)
        return;
    m_current = v;

    QString msg = QString("%1: %2").arg(tr("Vehicle selected")).arg(v->title());
    if (v->protocol()->squawk() > 0)
        msg.append(QString(" (%1)").arg(v->protocol()->squawkText()));
    v->message(msg, AppNotify::Important);

    //update JSengine
    App::setGlobalProperty("mandala", v->f_mandala);
    App::setContextProperty("mandala", v->f_mandala);

    emit currentChanged();
    emit vehicleSelected(v);
}

Vehicle *Vehicles::current(void) const
{
    return m_current;
}

void Vehicles::selectPrev()
{
    int i = indexOfChild(m_current);
    if (i < list_padding)
        i = list_padding;
    else if (i == list_padding)
        i = size() - 1;
    else if (i >= (size() - 1))
        i = list_padding;
    else
        i--;

    selectVehicle(qobject_cast<Vehicle *>(child(i)));
}
void Vehicles::selectNext()
{
    int i = indexOfChild(m_current);
    if (i < list_padding)
        i = list_padding;
    else if (i >= (size() - 1))
        i = list_padding;
    else
        i++;

    selectVehicle(qobject_cast<Vehicle *>(child(i)));
}

void Vehicles::jsSyncMandalaAccess(Fact *fact, QJSValue parent)
{
    // direct access to fact values from JS context
    // pure JS objects and data

    QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
    AppEngine *e = App::instance()->engine();

    MandalaFact *m = qobject_cast<MandalaFact *>(fact);

    if (fact->treeType() == Group) {
        QJSValue v;
        if (!m) {
            v = parent;
        } else {
            v = e->newObject(); //plain JS object
            parent.setProperty(fact->name(), v);
        }
        for (auto i : fact->facts()) {
            jsSyncMandalaAccess(i, v);
        }
        return;
    }

    QString mpath = m->mpath();
    QString s;
    s = QString("Object.defineProperty(%1,'%2',{get:function(){return "
                "apx.vehicles.current.mandala.%1.%2.value}, "
                "set:function(v){apx.vehicles.current.mandala.%1.%2.value=v}})")
            .arg(mpath.left(mpath.lastIndexOf('.')))
            .arg(fact->name());
    App::jsexec(s);
}
