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
#ifndef MandalaTreePlugin_H
#define MandalaTreePlugin_H

#include <App/AppRoot.h>
#include <App/PluginInterface.h>
#include <TreeModel/FactTreeView.h>
#include <QtCore>

#include <Mandala/MandalaTree.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
class SystreePlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.uavos.gcs.PluginInterface/1.0")
    Q_INTERFACES(PluginInterface)
public:
    QObject *createControl()
    {
        FactTreeWidget *w = new FactTreeWidget(Vehicles::instance()->current()->f_mandalatree,
                                               true,
                                               false);
        w->tree->expandToDepth(0);
        connect(Vehicles::instance(), &Vehicles::vehicleSelected, this, [w](Vehicle *v) {
            w->setRoot(v->f_mandalatree);
            w->tree->expandToDepth(0);
        });
        return w;
    }
    int flags() { return Widget | Restore | Launcher; }
    QString title() { return tr("Mandala tree"); }
    QString descr() { return tr("Mandala tree view"); }
    QString icon() { return "hexagon-multiple"; }
};
//=============================================================================
#endif
