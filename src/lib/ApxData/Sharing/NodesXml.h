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
#ifndef NodesXml_H
#define NodesXml_H
//=============================================================================
#include "ShareXml.h"
#include <Database/NodesReqVehicle.h>
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
    DBReqNodesLoadConfig req;
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
