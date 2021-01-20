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
#ifndef NodesXml_H
#define NodesXml_H
//=============================================================================
#include "ShareXml.h"
#include <Database/VehiclesReqVehicle.h>
#include <Dictionary/DictNode.h>
//=============================================================================
class NodesXmlExport : public ShareXmlExport
{
    Q_OBJECT
public:
    explicit NodesXmlExport(QString hash, QString title, QString fileName);

protected:
    bool run(QSqlQuery &query);
    bool write(QDomNode &dom);

private:
    DBReqVehiclesLoadConfig req;
    QVariantMap vehicleInfo;

    void writeNode(QDomNode &dom, const QVariantMap &data) const;
    void writeNodeField(QDomNode &dom, const DictNode::Field &f, const QVariantMap &values) const;
};
//=============================================================================
class NodesXmlImport : public ShareXmlImport
{
    Q_OBJECT
public:
    explicit NodesXmlImport(QString title, QString fileName);

protected:
    bool read(const QDomNode &dom);
    bool readOldFormat(const QDomNode &dom, int fmt);
    bool save(QSqlQuery &query);

    //result
public:
    QVariantMap vehicleInfo;
    QList<QVariantMap> nodes;
};
//=============================================================================
//=============================================================================
#endif
