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
#include "NodeItemBase.h"
#include "NodeTools.h"

#include <Protocols/ProtocolNode.h>

#include <QtCore>

class Nodes;

class NodeItem : public NodeItemBase
{
    Q_OBJECT
    Q_PROPERTY(QString sn READ sn CONSTANT)

    Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY versionChanged)
    Q_PROPERTY(QString hardware READ hardware WRITE setHardware NOTIFY hardwareChanged)

    Q_PROPERTY(bool identValid READ identValid WRITE setIdentValid NOTIFY identValidChanged)
    Q_PROPERTY(bool offline READ offline NOTIFY offlineChanged)

public:
    explicit NodeItem(Nodes *parent, QString sn, ProtocolNode *protocol);

    QList<NodeField *> allFields;
    QMap<QString, NodeField *> allFieldsByName;

    Nodes *nodes;
    //NodeTools *tools;

    void execCommand(quint16 cmd, const QString &name, const QString &descr);

    void setProtocol(ProtocolNode *protocol);

    //int loadConfigValues(QVariantMap values);
    //bool loadConfigValue(const QString &name, const QString &value);

protected:
    QTimer statusTimer;

    //override
    QVariant data(int col, int role) const;
    void hashData(QCryptographicHash *h) const;

private:
    xbus::node::ident::ident_s m_ident;

    ProtocolNode *m_protocol{nullptr};

    qint64 m_lastSeenTime{0};

    NodeField *m_status_field{nullptr};
    NodeItemBase *m_group{nullptr};

private slots:

    void validateDict();
    void validateData();

    void updateDescr();
    void updateStatus();

    void nodeNotify();

public slots:
    void upload();
    void clear();

    //protocols:
private slots:
    void identReceived();
    void dictReceived(const ProtocolNode::Dictionary &dict);
    void confReceived(const QVariantList &values);

    void messageReceived(xbus::node::msg::type_t type, QString msg);

signals:
    void requestIdent();
    void requestDict();
    void requestConf();
    void requestStatus();

    //---------------------------------------
    // PROPERTIES
public:
    QString sn() const;
    QString version() const;
    void setVersion(const QString &v);
    QString hardware() const;
    void setHardware(const QString &v);

    const xbus::node::ident::ident_s &ident() const;
    void setIdent(const xbus::node::ident::ident_s &v);

    bool identValid() const;
    void setIdentValid(const bool &v);
    bool offline() const;

protected:
    QString m_sn;
    QString m_version;
    QString m_hardware;
    bool m_identValid{false};

signals:
    void versionChanged();
    void hardwareChanged();
    void identChanged();
    void identValidChanged();
    void offlineChanged();
};
