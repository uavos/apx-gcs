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
    Q_PROPERTY(bool upgrading READ upgrading NOTIFY upgradingChanged)

public:
    explicit PNode(PNodes *parent, QString uid);

    QString uid() const { return m_uid; }

    enum msg_type_e {
        info,
        warn,
        err,
    };

    auto upgrading() const { return m_upgrading; }

private:
    QString m_uid;
    bool m_upgrading{};

    QTimer _upgradingDoneTimer;
    void setUpgrading(bool v);

private slots:
    void upgradeStarted(QString uid, QString name);
    void upgradeFinished(QString uid, bool success);

public slots:
    virtual void requestIdent() { _nimp(__FUNCTION__); }
    virtual void requestDict() { _nimp(__FUNCTION__); }
    virtual void requestConf() { _nimp(__FUNCTION__); }
    virtual void requestUpdate(QVariantMap values) { _nimp(__FUNCTION__); }

    virtual void requestReboot() { _nimp(__FUNCTION__); }
    virtual void requestShell(QStringList commands) { _nimp(__FUNCTION__); }
    virtual void requestUsr(quint8 cmd, QByteArray data) { _nimp(__FUNCTION__); }

signals:
    // a message from hardware
    void messageReceived(PNode::msg_type_e type, QString msg);

    // hash of node identity, title, version, hardware, etc
    void identReceived(QVariantMap ident);

    // list of QVariantMap describing fields {name, type, array, etc}
    void dictReceived(QVariantMap dict);

    // the whole set of parameters received
    void confReceived(QVariantMap values);

    // some parameters updated externally
    void confUpdated(QVariantMap values);

    // when requestUpdate accepted and saved
    void confSaved();

    //properties
    void upgradingChanged();
};
