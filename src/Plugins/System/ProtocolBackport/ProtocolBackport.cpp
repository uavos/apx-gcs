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
#include "ProtocolBackport.h"
#include "ProtocolV9.h"

#include <App/AppGcs.h>
#include <Protocols/ProtocolVehicles.h>

ProtocolBackport::ProtocolBackport(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Protocol backport"),
           tr("Compatible to v10.2 and below"),
           Bool)
{
    connect(this, &Fact::valueChanged, this, [this]() {
        if (value().toBool())
            install();
        else
            uninstall();
    });
    //setValue(true);
}

ProtocolBackport::~ProtocolBackport()
{
    uninstall();
}

void ProtocolBackport::install()
{
    apxMsgW() << title() << tr("installed");
    apxMsgW() << descr();
    if (m_converter)
        return;
    m_converter = new ProtocolV9(this);

    ProtocolVehicles *protocol = AppGcs::instance()->protocol;
    protocol->setConverter(m_converter);
}

void ProtocolBackport::uninstall()
{
    apxMsgW() << title() << tr("removed");
    if (!m_converter)
        return;
    delete m_converter;
    m_converter = nullptr;
    ProtocolVehicles *protocol = AppGcs::instance()->protocol;
    protocol->setConverter(nullptr);
}
