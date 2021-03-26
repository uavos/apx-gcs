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

#include "NodeField.h"
#include "NodeTools.h"

#include <Protocols/Protocols.h>

#include <App/AppNotify.h>

#include <QtCore>

class Nodes;

class NodeItem : public Fact
{
    Q_OBJECT

public:
    explicit NodeItem(Fact *parent, Nodes *nodes, PNode *protocol);

    NodeTools *tools;

    //int loadConfigValues(QVariantMap values);
    bool loadConfigValue(const QString &name, const QString &value);

    const QList<NodeField *> &fields() const;

    Q_INVOKABLE void message(QString msg,
                             AppNotify::NotifyFlags flags = AppNotify::FromVehicle
                                                            | AppNotify::Important);

    inline Nodes *nodes() const { return _nodes; }

    PNode *protocol() const { return _protocol; }

protected:
    QTimer statusTimer;

    QVariant data(int col, int role) const override;
    QString toolTip() const override;

private:
    Nodes *_nodes{nullptr};
    PNode *_protocol;

    QJsonObject _ident;

    QList<NodeField *> m_fields;

    QJsonArray _parameters;
    void updateMetadataAPXFW(Fact *root, Fact *group, QJsonValue json);

    NodeField *m_status_field{nullptr};
    void groupArrays();
    void groupArrays(Fact *group);
    void updateArrayRowDescr(Fact *fRow);
    void removeEmptyGroups(Fact *f);
    void linkGroupValues(Fact *f);
    void updateArrayChBinding(Fact *f_element, Fact *f_array, Fact *f_ch);

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
    void identReceived(QJsonValue json);
    void dictReceived(const ProtocolNode::Dict &dict);
    void confReceived(const QVariantMap &values);
    void confSaved();

    void messageReceived(PNode::msg_type_e type, QString msg);
    void statusReceived(const xbus::node::status::status_s &status);

signals:
    void saveValues();
    void shell(QStringList commands);
};
