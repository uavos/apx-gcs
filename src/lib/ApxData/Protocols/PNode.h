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

#include "PBase.h"

class PVehicle;
class PNode;
class PNodes;

class PNode : public PTreeBase
{
    Q_OBJECT
    Q_PROPERTY(QString uid READ uid CONSTANT)

public:
    explicit PNode(PNodes *parent, QString uid);

    QString uid() const { return m_uid; }

    enum msg_type_e {
        info,
        warn,
        err,
    };

private:
    QString m_uid;

public slots:
    virtual void requestIdent() { _nimp(__FUNCTION__); }
    virtual void requestDict() { _nimp(__FUNCTION__); }
    virtual void requestConf() { _nimp(__FUNCTION__); }
    virtual void requestScript() { _nimp(__FUNCTION__); }
    virtual void requestUpdate(QVariantMap values) { _nimp(__FUNCTION__); }

    virtual void requestReboot() { _nimp(__FUNCTION__); }

signals:
    void messageReceived(PNode::msg_type_e type, QString msg);
    void identReceived(QJsonValue json);
    void dictReceived(QJsonValue json);
    void confReceived(QVariantMap values);
    void confSaved(); // when requestUpdate accepted and saved
};
