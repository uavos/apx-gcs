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
#include "NodesShare.h"
#include "Nodes.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>

NodesShare::NodesShare(Nodes *nodes, Fact *parent, Flags flags)
    : Share(parent, "nodes", tr("Nodes"), AppDirs::configs(), flags)
    , _nodes(nodes)
{}

QString NodesShare::getDefaultTitle()
{
    QStringList st;
    st << _nodes->vehicle->title();
    st << _nodes->getConfigTitle();
    return st.join('-');
}
bool NodesShare::exportRequest(QString format, QString fileName)
{
    if (!saveData(_nodes->toJsonDocument().toJson(), fileName))
        return false;
    _exported(fileName);
    return true;
}
bool NodesShare::importRequest(QString format, QString fileName)
{
    if (!_nodes->fromJsonDocument(loadData(fileName)))
        return false;
    _imported(fileName);
    return true;
}
