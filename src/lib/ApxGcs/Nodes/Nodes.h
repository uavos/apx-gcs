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

#include <ApxMisc/DelayedEvent.h>
#include <Vehicles/Vehicle.h>

#include "NodeItem.h"

class Vehicle;
class LookupConfigs;

class Nodes : public Fact
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
    explicit Nodes(Vehicle *vehicle);

    Vehicle *vehicle;

    Fact *f_upload;
    Fact *f_search;
    Fact *f_stop;
    Fact *f_reload;
    Fact *f_clear;
    Fact *f_status;

    NodeItem *node(const QString &uid) const;
    QList<NodeItem *> nodes() const { return m_nodes; }

    NodeItem *add(PNode *protocol);

    Q_INVOKABLE void shell(QStringList commands);

    bool valid() const { return m_valid; }

    QString getConfigTitle();

    //Fact override
    QVariant toVariant() const override;
    void fromVariant(const QVariant &var) override;

private:
    PNodes *_protocol;

    QList<NodeItem *> m_nodes;
    QDateTime m_syncTimestamp;

    bool m_valid{};

    DelayedEvent _updateActions{100, true};

private slots:
    void updateActions();
    void updateValid();

    void search();
    void clear();
    void reload();
    void upload();
    void stop();

    void node_available(PNode *node);
    void syncDone();

signals:
    void validChanged();
};
