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
#include "Vehicles.h"
#include "VehicleSelect.h"
#include "VehicleWarnings.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <Database/TelemetryDB.h>
#include <QQmlEngine>

APX_LOGGING_CATEGORY(VehiclesLog, "core.vehicles")
Vehicles *Vehicles::_instance = nullptr;

Vehicles::Vehicles(Fact *parent, Protocols *protocols)
    : Fact(parent, "vehicles", tr("Vehicles"), tr("Discovered vehicles"), Group | Count, "drone")
{
    _instance = this;

    qmlRegisterUncreatableType<Vehicles>("APX.Vehicles", 1, 0, "Vehicles", "Reference only");
    qmlRegisterUncreatableType<Vehicle>("APX.Vehicles", 1, 0, "Vehicle", "Reference only");
    qmlRegisterUncreatableType<VehicleWarnings>("APX.Vehicles",
                                                1,
                                                0,
                                                "VehicleWarnings",
                                                "Reference only");

    qmlRegisterUncreatableType<PVehicle>("APX.Vehicles", 1, 0, "PVehicle", "Reference only");

    f_select = new VehicleSelect(this,
                                 "select",
                                 tr("Select vehicle"),
                                 tr("Change the active vehicle"));
    f_select->setIcon("select");
    f_select->setTreeType(Action);
    connect(f_select, &VehicleSelect::vehicleSelected, this, &Vehicles::selectVehicle);

    auto f_clear = new Fact(this,
                            "clear",
                            tr("Clear"),
                            tr("Remove all vehicles"),
                            Action,
                            "notification-clear-all");
    connect(f_clear, &Fact::triggered, this, &Vehicles::clear);

    f_replay = new Vehicle(this, nullptr);
    f_select->addVehicle(f_replay);

    AppRoot::instance()->setMandala(f_replay->mandala());

    //JS register mandala
    //if (App::instance()->engine()) {

    Mandala *m = f_replay->f_mandala;
    //register mandala constants for QML and JS
    for (auto s : m->constants.keys()) {
        const QVariant &v = m->constants.value(s);
        //JSEngine layer
        App::setGlobalProperty(s, v);
        //QmlEngine layer
        App::setContextProperty(s, v);
    }

    _jsSyncMandalaAccess(m, App::instance()->engine()->globalObject());
    for (auto f : m->facts()) {
        App::instance()->engine()->jsProtectObjects(static_cast<MandalaFact *>(f)->mpath());
    }
    //}

    //Database register fields
    DatabaseRequest::Records recMandala;
    recMandala.names << "id"
                     << "name"
                     << "title"
                     << "units";
    for (auto f : m->uid_map.values()) {
        if (f->isSystem())
            continue;
        QVariantList v;
        v << f->meta().uid;
        v << f->mpath();
        v << f->meta().title;
        v << f->meta().units;
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

    selectVehicle(f_replay);

    //connect protocols
    connect(protocols, &Protocols::vehicle_available, this, &Vehicles::vehicle_available);
}

void Vehicles::vehicle_available(PVehicle *protocol)
{
    Vehicle *v = new Vehicle(this, protocol);

    emit vehicleRegistered(v);

    if (v->isIdentified()) {
        QString msg = QString("%1: %2").arg(tr("Vehicle identified")).arg(v->title());
        v->message(msg, AppNotify::Important);
    }

    if (!current() || v->isLocal()) {
        selectVehicle(v);
    } else {
        // select identified vehicle
        while (v->isIdentified()) {
            if (current()->isIdentified() && !current()->isGroundControl())
                break;
            selectVehicle(v);
            break;
        }
    }
}

void Vehicles::selectVehicle(Vehicle *v)
{
    if (!v) {
        selectVehicle(f_replay);
        return;
    }

    if (m_current) {
        m_current->setActive(false);
    }

    v->setActive(true); //ensure is active

    if (m_current == v)
        return;
    m_current = v;

    QString msg = QString("%1: %2").arg(tr("Vehicle selected")).arg(v->title());
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

void Vehicles::deleteVehicle(Vehicle *v)
{
    if (!v)
        return;

    if (!v->isIdentified()) {
        v->message(tr("Vehicle can't be deleted"));
        return;
    }

    // select another vehicle
    if (v->active()) {
        for (auto i : facts()) {
            auto f = static_cast<Vehicle *>(i);
            if (f->active() || f == v)
                continue;
            if (!f->isIdentified())
                continue;
            selectVehicle(f);
            break;
        }
        if (v->active()) {
            for (auto i : facts()) {
                auto f = static_cast<Vehicle *>(i);
                if (f->active() || f == v)
                    continue;
                if (!f->isLocal())
                    continue;
                selectVehicle(f);
                break;
            }
        }
        if (v->active())
            return;
    }
    v->message(tr("Vehicle deleted"));
    // remove from models
    v->setParentFact(nullptr);
    v->menuBack();
    // delay actual delete operation
    QTimer::singleShot(1000, this, [v]() { v->deleteFact(); });
}
void Vehicles::clear()
{
    for (auto i : facts()) {
        auto f = static_cast<Vehicle *>(i);
        if (!f->isIdentified())
            continue;
        deleteVehicle(f);
    }
}

void Vehicles::_jsSyncMandalaAccess(Fact *fact, QJSValue parent)
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
            e->jsProtectPropertyWrite(m->mpath());
        }
        for (auto i : fact->facts()) {
            _jsSyncMandalaAccess(i, v);
        }
        return;
    }

    if (!m)
        return;

    QString mpath = m->mpath();
    QString s = QString("Object.defineProperty(%1,'%2',{get:function(){return "
                        "apx.vehicles.current.mandala.%1.%2.value},"
                        "set:function(v){apx.vehicles.current.mandala.%1.%2.value=v}})")
                    .arg(m->mpath().left(mpath.lastIndexOf('.')))
                    .arg(fact->name());

    App::jsexec(s);
}
