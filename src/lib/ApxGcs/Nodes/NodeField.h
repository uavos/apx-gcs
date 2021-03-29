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

#include "NodeScript.h"

class NodeItem;

class NodeField : public Fact
{
    Q_OBJECT

    Q_PROPERTY(NodeScript *script MEMBER _script);

public:
    explicit NodeField(Fact *parent,
                       NodeItem *node,
                       QJsonObject json,
                       size_t id,
                       NodeField *arrayParent = nullptr);

    QVariant confValue(void) const;
    void setConfValue(const QVariant &v);

    //Fact override
    virtual QString toolTip() const override;
    virtual QString toText(const QVariant &v) const override;

    inline void setHelp(const QString &s) { _help = s; }
    inline QString fpath() const { return _fpath; }
    inline QString type() const { return _type; }

private:
    NodeItem *_node;

    QString _type;
    size_t _id;
    size_t _array; // array index

    QString _fpath;
    QString _help;

    NodeScript *_script{};

private slots:
    void updateStatus();
};
