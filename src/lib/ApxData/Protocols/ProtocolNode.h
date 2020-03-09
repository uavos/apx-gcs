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

#include "ProtocolBase.h"
#include "ProtocolNodeRequest.h"

#include <QtCore>

#include <Xbus/XbusNode.h>

class ProtocolNodes;

class ProtocolNode : public ProtocolBase
{
    Q_OBJECT
public:
    explicit ProtocolNode(ProtocolNodes *nodes, const QString &sn);

    //service protocol and requests management
    void downlink(xbus::pid_t pid, ProtocolStreamReader &stream);

    const xbus::node::ident::ident_s &ident() const;

    struct field_s : public xbus::node::dict::field_s
    {
        QString name;
        QString descr;
        QStringList units;
    };
    typedef QList<field_s> Dictionary;

private:
    QString sn;
    ProtocolNodes *nodes;

    xbus::node::ident::ident_s m_ident;
    bool ident_valid{false};

    ProtocolNodeRequest *make_request(xbus::pid_t pid, int timeout_ms = 0, int retry_cnt = 0);

    //export signals and slots
signals:
    void requestTimeout(quint16 cmd, QByteArray data);

    void identReceived(xbus::node::ident::ident_s ident);
    void dictReceived(const ProtocolNode::Dictionary &dict);
    void confReceived(const QVariantList &values);

    void messageReceived(xbus::node::msg::type_t type, QString msg);

public slots:
    void requestIdent();
    void requestDict();
    void requestConf();
    void requestStatus();
};
