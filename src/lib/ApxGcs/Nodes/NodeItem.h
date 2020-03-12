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
#include <Protocols/ProtocolViewBase.h>

#include <QtCore>

class Nodes;

class NodeItem : public ProtocolViewBase<ProtocolNode>
{
    Q_OBJECT
    Q_PROPERTY(QString sn READ sn CONSTANT)

    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QString hardware READ hardware NOTIFY hardwareChanged)

    Q_PROPERTY(bool identValid READ identValid WRITE setIdentValid NOTIFY identValidChanged)
    Q_PROPERTY(bool dictValid READ dictValid WRITE setDictValid NOTIFY dictValidChanged)
    Q_PROPERTY(bool dataValid READ dataValid WRITE setDataValid NOTIFY dataValidChanged)

    Q_PROPERTY(bool upgrading READ upgrading WRITE setUpgrading NOTIFY upgradingChanged)

public:
    explicit NodeItem(Fact *parent, ProtocolNode *protocol);

    QList<NodeField *> allFields;
    QMap<QString, NodeField *> allFieldsByName;

    //NodeTools *tools;

    void execCommand(quint16 cmd, const QString &name, const QString &descr);

    //int loadConfigValues(QVariantMap values);
    //bool loadConfigValue(const QString &name, const QString &value);

protected:
    QTimer statusTimer;

    //override
    QVariant data(int col, int role) const;
    void hashData(QCryptographicHash *h) const;

private:
    qint64 m_lastSeenTime{0};

    NodeField *m_status_field{nullptr};
    NodeItemBase *m_group{nullptr};

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
    QString hardware() const;

    const xbus::node::ident::ident_s &ident() const;

    bool identValid() const;
    void setIdentValid(const bool &v);

    bool dictValid() const;
    void setDictValid(const bool &v);

    bool dataValid() const;
    void setDataValid(const bool &v);

    bool upgrading() const;
    void setUpgrading(const bool &v);

protected:
    bool m_identValid{false};
    bool m_dictValid{false};
    bool m_dataValid{false};
    bool m_upgrading{false};

signals:
    void identChanged();
    void versionChanged();
    void hardwareChanged();

    void identValidChanged();
    void dictValidChanged();
    void dataValidChanged();

    void upgradingChanged();
};
