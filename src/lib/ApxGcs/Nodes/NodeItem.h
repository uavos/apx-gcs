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
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
    explicit NodeItem(Fact *parent, Nodes *nodes, PNode *protocol);

    NodeTools *tools;

    const QList<NodeField *> &fields() const { return m_fields; }
    Nodes *nodes() const { return _nodes; }
    PNode *protocol() const { return _protocol; }
    bool valid() const { return m_valid; }

    bool loadConfigValue(const QString &name, const QString &value);

    Q_INVOKABLE void message(QString msg,
                             AppNotify::NotifyFlags flags = AppNotify::FromVehicle
                                                            | AppNotify::Important);

protected:
    QTimer statusTimer;

    QVariant data(int col, int role) const override;
    QString toolTip() const override;

private:
    Nodes *_nodes;
    PNode *_protocol;

    QVariantMap _ident;

    QList<NodeField *> m_fields;

    QJsonArray _parameters;
    void updateMetadataAPXFW(Fact *root, Fact *group, QJsonValue json);

    NodeField *_status_field{nullptr};

    void groupArrays();
    void groupArrays(Fact *group);
    void updateArrayRowDescr(Fact *fRow);
    void removeEmptyGroups(Fact *f);
    void linkGroupValues(Fact *f);
    void updateArrayChBinding(Fact *f_element, Fact *f_array, Fact *f_ch);

    bool m_valid{};

private slots:
    void validateData();

    void updateDescr();
    void updateStatus();

public slots:
    void upload();
    void clear();

    //protocols:
private slots:
    void identReceived(QVariantMap ident);
    void dictReceived(QVariantList dict);
    void confReceived(QVariantMap values);
    void confUpdated(QVariantMap values);
    void confSaved();

    void messageReceived(PNode::msg_type_e type, QString msg);
    void statusReceived(const xbus::node::status::status_s &status);

signals:
    void saveValues();
    void shell(QStringList commands);

    void validChanged();
};
