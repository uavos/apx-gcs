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

#include <Fact/Fact.h>
#include <QtCore>

#include <Protocols/ProtocolNode.h>

#include "NodeScript.h"

class NodeItem;

class NodeField : public Fact
{
    Q_OBJECT

    Q_PROPERTY(NodeScript *script MEMBER _script);

public:
    explicit NodeField(Fact *parent,
                       NodeItem *node,
                       xbus::node::conf::fid_t fid,
                       const ProtocolNode::dict_field_s &field,
                       NodeField *parentField = nullptr);

    QVariant confValue(void) const;
    void setConfValue(const QVariant &v);

    //Fact override
    virtual QString toolTip() const override;
    virtual QString toText(const QVariant &v) const override;

    inline void setHelp(const QString &s) { _help = s; }
    inline QString fpath() const { return _fpath; }

    inline xbus::node::conf::fid_t fid() const { return _fid; }
    inline xbus::node::conf::type_e type() const { return _type; }

private:
    NodeItem *_node;

    xbus::node::conf::fid_t _fid;
    xbus::node::conf::type_e _type;

    QString _fpath;
    QString _help;

    NodeScript *_script{};

private slots:
    void updateStatus();
};
