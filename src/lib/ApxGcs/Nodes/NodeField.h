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
#pragma once

#include <Fact/Fact.h>
#include <QtCore>

#include <Protocols/ProtocolNode.h>

class NodeItem;
class PawnCompiler;

class NodeField : public Fact
{
    Q_OBJECT

public:
    explicit NodeField(Fact *parent,
                       NodeItem *node,
                       xbus::node::conf::fid_t fid,
                       const ProtocolNode::dict_field_s &field,
                       NodeField *parentField = nullptr);

    xbus::node::conf::fid_t fid() const;
    QVariant uploadableValue(void) const;

    //values used for storage and share
    QString toString() const;
    void fromString(const QString &s);

    //Fact override
    virtual bool setValue(const QVariant &v) override;
    virtual QVariant data(int col, int role) const override;
    virtual QString toolTip() const override;

    inline void setHelp(const QString &s) { _help = s; }
    inline QString fpath() const { return _fpath; }

private:
    NodeItem *_node;
    NodeField *_parentField;

    xbus::node::conf::fid_t m_fid;
    xbus::node::conf::type_e _type;

    PawnCompiler *pawncc{nullptr};
    QByteArray scriptCodeSave;

    QString _fpath;
    QString _help;

private slots:
    void updateStatus();
};
