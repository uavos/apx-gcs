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
#include <App/App.h>

#include "PBase.h"
#include "Protocols.h"

Protocols::Protocols(Fact *parent)
    : Fact(parent, "protocols", tr("Protocols"), tr("Data exchange interfaces"), Group, "contain")
{
    f_current = new Fact(this,
                         "current",
                         tr("Current protocol"),
                         tr("Active data format"),
                         Text | PersistentValue);

    f_current->setDefaultValue("papx");
    connect(this, &Fact::sizeChanged, this, &Protocols::updateNames, Qt::QueuedConnection);

    bindProperty(f_current, "value", true); // just for visual feedback
    connect(f_current, &Fact::valueChanged, this, &Protocols::updateProtocol, Qt::QueuedConnection);

    connect(App::instance(), &App::loadingFinished, this, &Protocols::updateProtocol);
}

void Protocols::updateNames()
{
    QStringList list;
    for (auto i : findFacts<PBase>())
        list.append(i->name());
    f_current->setEnumStrings(list);
}

void Protocols::updateProtocol()
{
    PBase *protocol = nullptr;
    const QString s = f_current->text();
    for (auto i : findFacts<PBase>()) {
        if (i->name() != s)
            continue;
        protocol = i;
        break;
    }
    if (!protocol) {
        apxMsgW() << tr("Can't select protocol").append(':') << s;
        return;
    }
    if (_protocol == protocol)
        return;

    bool chg = _protocol;
    if (chg) {
        _protocol->trace()->enable(false);
        disconnect(_protocol, nullptr, this, nullptr);
        _protocol = nullptr;
    }
    _protocol = protocol;
    qDebug() << "Protocol:" << s << _protocol->title();
    if (chg || s != f_current->defaultValue().toString()) {
        apxMsg() << tr("Selected protocol").append(':') << s;
    }

    // connect protocol interface
    connect(_protocol, &PBase::tx_data, this, &Protocols::tx_data);
    connect(_protocol, &PBase::unit_available, this, &Protocols::unit_available);
    connect(_protocol->trace(), &PTrace::packet, this, &Protocols::trace_packet);
}

void Protocols::rx_data(QByteArray packet)
{
    if (_protocol)
        _protocol->rx_data(packet);
}

void Protocols::setTraceEnabled(bool v)
{
    if (_protocol)
        _protocol->trace()->enable(v);
}
