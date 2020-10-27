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

#include <App/AppRoot.h>
#include <App/PluginInterface.h>
#include <TreeModel/FactTreeView.h>
#include <QtCore>

#include <Mandala/Mandala.h>
#include <Vehicles/Vehicles.h>

class MandalaTreePlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.uavos.gcs.PluginInterface/1.0")
    Q_INTERFACES(PluginInterface)
public:
    QObject *createControl()
    {
        FactTreeWidget *w = new FactTreeWidget(Vehicles::instance()->current()->f_mandala,
                                               true,
                                               false);
        w->tree->expandToDepth(0);
        connect(Vehicles::instance(), &Vehicles::vehicleSelected, w, [w](Vehicle *v) {
            w->setRoot(v->f_mandala);
            w->tree->expandToDepth(0);
        });
        return w;
    }
    int flags() { return Widget | Restore | Launcher; }
    QString title() { return tr("Mandala"); }
    QString descr() { return tr("Mandala tree view"); }
    QString icon() { return "hexagon-multiple"; }
};
