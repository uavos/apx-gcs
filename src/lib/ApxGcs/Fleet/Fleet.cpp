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
#include "Fleet.h"
#include "UnitSelect.h"
#include "UnitWarnings.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <QQmlEngine>

APX_LOGGING_CATEGORY(FleetLog, "core.fleet")
Fleet *Fleet::_instance = nullptr;

Fleet::Fleet(Fact *parent, Protocols *protocols)
    : Fact(parent, "fleet", tr("Fleet"), tr("Discovered units"), Group | Count, "drone")
{
    _instance = this;

    qmlRegisterUncreatableType<Fleet>("APX.Fleet", 1, 0, "Fleet", "Reference only");
    qmlRegisterUncreatableType<Unit>("APX.Fleet", 1, 0, "Unit", "Reference only");
    qmlRegisterUncreatableType<UnitWarnings>("APX.Fleet", 1, 0, "UnitWarnings", "Reference only");

    qmlRegisterUncreatableType<PUnit>("APX.Fleet", 1, 0, "PUnit", "Reference only");

    f_select = new UnitSelect(this, "select", tr("Select unit"), tr("Change the active unit"));
    f_select->setIcon("select");
    f_select->setTreeType(Action);
    connect(f_select, &UnitSelect::unitSelected, this, &Fleet::selectUnit);

    auto f_clear = new Fact(this,
                            "clear",
                            tr("Clear"),
                            tr("Remove all fleet units"),
                            Action,
                            "notification-clear-all");
    connect(f_clear, &Fact::triggered, this, &Fleet::clearAll);

    f_replay = new Unit(this, nullptr);
    f_select->addUnit(f_replay);

    AppRoot::instance()->setMandala(f_replay->mandala());

    //JS register mandala
    Mandala *m = f_replay->f_mandala;
    _jsSyncMandalaAccess(m, App::instance()->engine()->globalObject());
    for (auto f : m->facts()) {
        App::instance()->engine()->jsProtectObjects(static_cast<MandalaFact *>(f)->mpath());
    }

    selectUnit(f_replay);

    //connect protocols
    connect(protocols, &Protocols::unit_available, this, &Fleet::unit_available);
}

void Fleet::unit_available(PUnit *protocol)
{
    auto unit = new Unit(this, protocol);

    emit unitRegistered(unit);

    if (!m_gcs || unit->isLocal() || (!m_gcs->isGroundControl() && unit->isGroundControl()))
        _update_gcs(unit);

    if (unit->isIdentified()) {
        QString msg = QString("%1: %2").arg(tr("Unit identified")).arg(unit->title());
        unit->message(msg, AppNotify::Important);
    }

    if (!current() || unit->isLocal()) {
        selectUnit(unit);
    } else {
        // select identified unit
        while (unit->isIdentified()) {
            if (current()->isIdentified() && !current()->isGroundControl())
                break;
            selectUnit(unit);
            break;
        }
    }
}

void Fleet::selectUnit(Unit *unit)
{
    if (!unit) {
        selectUnit(f_replay);
        return;
    }

    if (m_current) {
        m_current->setActive(false);
    }

    unit->setActive(true); //ensure is active

    if (m_current == unit)
        return;
    m_current = unit;

    //update JSengine
    App::setGlobalProperty("mandala", unit->f_mandala);
    App::setContextProperty("mandala", unit->f_mandala);

    if (size() > list_padding) {
        QString msg = QString("%1: %2").arg(tr("Unit selected")).arg(unit->title());
        unit->message(msg, AppNotify::Important);
    }

    emit currentChanged();
    emit unitSelected(unit);

    if (unit->isGroundControl())
        _update_gcs(unit);
}

void Fleet::selectPrev()
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

    selectUnit(qobject_cast<Unit *>(child(i)));
}
void Fleet::selectNext()
{
    int i = indexOfChild(m_current);
    if (i < list_padding)
        i = list_padding;
    else if (i >= (size() - 1))
        i = list_padding;
    else
        i++;

    selectUnit(qobject_cast<Unit *>(child(i)));
}

void Fleet::deleteUnit(Unit *unit)
{
    if (!unit)
        return;

    if (!unit->isIdentified()) {
        unit->message(tr("Unit can't be deleted"));
        return;
    }

    // select another unit
    if (unit->active()) {
        for (auto i : facts()) {
            auto f = static_cast<Unit *>(i);
            if (f->active() || f == unit)
                continue;
            if (!f->isIdentified())
                continue;
            selectUnit(f);
            break;
        }
        if (unit->active()) {
            for (auto i : facts()) {
                auto f = static_cast<Unit *>(i);
                if (f->active() || f == unit)
                    continue;
                if (!f->isLocal())
                    continue;
                selectUnit(f);
                break;
            }
        }
        if (unit->active())
            return;
    }
    unit->message(tr("Unit deleted"));
    // remove from models
    unit->setParentFact(nullptr);
    unit->menuBack();
    // delay actual delete operation
    QTimer::singleShot(1000, this, [unit]() { unit->deleteFact(); });
}
void Fleet::clearAll()
{
    for (auto i : facts()) {
        auto f = static_cast<Unit *>(i);
        if (!f->isIdentified())
            continue;
        deleteUnit(f);
    }
}

void Fleet::_jsSyncMandalaAccess(Fact *fact, QJSValue parent)
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
                        "apx.fleet.current.mandala.%1.%2.value},"
                        "set:function(v){apx.fleet.current.mandala.%1.%2.value=v}})")
                    .arg(m->mpath().left(mpath.lastIndexOf('.')))
                    .arg(fact->name());

    App::jsexec(s);
}

void Fleet::_update_gcs(Unit *unit)
{
    if (m_gcs == unit)
        return;
    m_gcs = unit;
    emit gcsChanged();
}
