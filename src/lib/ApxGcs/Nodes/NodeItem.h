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

#include "NodeField.h"
#include "NodeTools.h"

#include <Protocols/ProtocolNode.h>
#include <Protocols/ProtocolViewBase.h>

#include <QtCore>

class Nodes;

class NodeItem : public ProtocolViewBase<ProtocolNode>
{
    Q_OBJECT

public:
    explicit NodeItem(Fact *parent, ProtocolNode *protocol);

    //NodeTools *tools;

    void execCommand(quint16 cmd, const QString &name, const QString &descr);

    //int loadConfigValues(QVariantMap values);
    //bool loadConfigValue(const QString &name, const QString &value);

    const QList<NodeField *> &fields() const;

protected:
    QTimer statusTimer;

    QVariant data(int col, int role) const override;

private:
    qint64 m_lastSeenTime{0};

    QList<NodeField *> m_fields;

    NodeField *m_status_field{nullptr};
    void groupArrays();
    void groupArrays(Fact *group);
    void updateArrayRowDescr(Fact *fRow);

private slots:

    void validateDict();
    void validateData();

    void updateDescr();
    void updateStatus();

public slots:
    void upload();
    void clear();

    //protocols:
private slots:
    void identReceived();
    void dictReceived(const ProtocolNode::Dict &dict);
    void confReceived(const QVariantList &values);
    void confSaved();

    void messageReceived(xbus::node::msg::type_t type, QString msg);

signals:
    void saveValues();
};
