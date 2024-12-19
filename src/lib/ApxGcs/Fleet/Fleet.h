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

#include <Protocols/Protocols.h>

#include "Unit.h"
#include "UnitSelect.h"

#include <App/AppEngine.h>

class Unit;

class Fleet : public Fact
{
    Q_OBJECT

    Q_PROPERTY(Unit *current READ current NOTIFY currentChanged)
    Q_PROPERTY(Unit *gcs READ gcs NOTIFY gcsChanged)

public:
    explicit Fleet(Fact *parent, Protocols *protocols);

    static Fleet *instance() { return _instance; }

    static auto replay() { return instance()->f_replay; }
    static auto select() { return instance()->f_select; }

    static constexpr const int list_padding = 2;

public:
    Unit *current(void) const { return m_current; }
    Unit *gcs(void) const { return m_gcs; }

protected:
    QPointer<Unit> m_current;
    QPointer<Unit> m_gcs;

private:
    static Fleet *_instance;
    Unit *f_replay;
    UnitSelect *f_select;

    void _jsSyncMandalaAccess(Fact *fact, QJSValue parent);

    void _update_gcs(Unit *unit);

public slots:
    void selectUnit(Unit *v);
    void selectPrev();
    void selectNext();

    void deleteUnit(Unit *v);
    void clearAll();

    //data connection
private slots:
    void unit_available(PUnit *protocol);

signals:
    void unitRegistered(Unit *unit);
    void unitRemoved(Unit *unit);
    void unitSelected(Unit *unit);

signals:
    void currentChanged();
    void gcsChanged();
};
