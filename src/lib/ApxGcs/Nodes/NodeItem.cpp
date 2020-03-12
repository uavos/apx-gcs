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
#include "NodeItem.h"
#include "NodeField.h"
#include "NodeTools.h"
#include "Nodes.h"
#include <QtSql>

#include <App/App.h>
#include <Database/NodesDB.h>
#include <Vehicles/VehicleWarnings.h>
#include <Vehicles/Vehicles.h>
#include <QFontDatabase>
#include <QQmlEngine>

NodeItem::NodeItem(Fact *parent, ProtocolNode *protocol)
    : ProtocolViewBase(parent, protocol)
{
    qmlRegisterUncreatableType<NodeItem>("APX.Node", 1, 0, "Node", "Reference only");

    //tools = new NodeTools(this, Action);

    connect(this, &NodeItem::versionChanged, this, &NodeItem::updateDescr);
    connect(this, &NodeItem::hardwareChanged, this, &NodeItem::updateDescr);
    connect(this, &NodeItem::upgradingChanged, this, &NodeItem::updateDescr);
    connect(this, &NodeItem::identChanged, this, &NodeItem::updateDescr);

    //validity
    connect(this, &NodeItem::identChanged, this, &NodeItem::clear);
    connect(this, &NodeItem::dictValidChanged, this, &NodeItem::validateDict);
    connect(this, &NodeItem::dataValidChanged, this, &NodeItem::validateData);

    //protocol
    connect(protocol, &QObject::destroyed, this, [this]() { deleteLater(); });

    connect(protocol, &ProtocolNode::enabledChanged, this, &NodeItem::updateDescr);

    // responses mapping
    connect(protocol, &ProtocolNode::identReceived, this, &NodeItem::identReceived);
    connect(protocol, &ProtocolNode::dictReceived, this, &NodeItem::dictReceived);
    connect(protocol, &ProtocolNode::confReceived, this, &NodeItem::confReceived);
    connect(protocol, &ProtocolNode::messageReceived, this, &NodeItem::messageReceived);

    // requests mapping
    connect(this, &NodeItem::requestIdent, protocol, &ProtocolNode::requestIdent);
    connect(this, &NodeItem::requestDict, protocol, &ProtocolNode::requestDict);
    connect(this, &NodeItem::requestConf, protocol, &ProtocolNode::requestConf);
    connect(this, &NodeItem::requestStatus, protocol, &ProtocolNode::requestStatus);

    // props forward
    connect(protocol, &ProtocolNode::identChanged, this, &NodeItem::identChanged);
    connect(protocol, &ProtocolNode::versionChanged, this, &NodeItem::versionChanged);
    connect(protocol, &ProtocolNode::hardwareChanged, this, &NodeItem::hardwareChanged);

    //FIXME: nodes->storage->loadNodeInfo(this);

    statusTimer.setSingleShot(true);
    statusTimer.setInterval(10000);
    connect(&statusTimer, &QTimer::timeout, this, &NodeItem::updateDescr);
}

void NodeItem::validateDict()
{
    if (!dictValid())
        return;
    //groupFields();
    //qDebug()<path();

    //fields map
    for (int i = 0; i < allFields.size(); ++i) {
        NodeField *f = allFields.at(i);
        //expand complex numbers
        bool bComplex = f->size() > 0;
        if (bComplex) {
            for (int i2 = 0; i2 < f->size(); ++i2) {
                NodeField *f2 = static_cast<NodeField *>(f->child(i2));
                allFieldsByName.insert(f2->name(), f2);
            }
        } else
            allFieldsByName.insert(f->name(), f);
    }
}
void NodeItem::validateData()
{
    if (!dataValid())
        return;
    if (!protocol()->enabled())
        return;
    if (ident().flags.bits.reconf) {
        //nodes->storage->restoreNodeConfig(this);
    } else {
        //setNconfID(0);
        //nodes->storage->saveNodeConfig(this);
    }
    //qDebug()<<"Node dataValid"<<path();
}

void NodeItem::updateDescr()
{
    statusTimer.stop();
    setActive(false);
    QStringList st;
    st.append(hardware());
    if (version() != App::version())
        st.append(version());
    if (!protocol()->enabled())
        st.append(tr("offline"));
    if (ident().flags.bits.files == 1 && protocol()->file("fw"))
        st.append("LOADER");
    //st.append(sn());
    setDescr(st.join(' '));
}
void NodeItem::updateStatus()
{
    if (ident().flags.bits.reconf) {
        setValue(tr("no config").toUpper());
        return;
    }
    if (m_status_field) {
        setValue(m_status_field->text().trimmed());
    }
}

void NodeItem::clear()
{
    m_status_field = nullptr;
    //tools->clearCommands();
    allFields.clear();
    allFieldsByName.clear();
    removeAll();
    setDictValid(false);
    setDataValid(false);
    setModified(false);
}

void NodeItem::upload()
{
    if (!(dictValid() && dataValid()))
        return;
    if (!modified())
        return;
    //saveTelemetryUploadEvent();
    int cnt = 0;
    /*foreach (NodeField *f, allFields) {
        if (!f->modified())
            continue;
        emit uploadValue(f->id, f->uploadableValue());
        cnt++;
    }
    if (cnt > 0)
        emit saveValues();*/
}

QVariant NodeItem::data(int col, int role) const
{
    if (dictValid() && dataValid() && (!upgrading())) {
        switch (role) {
        case Qt::ForegroundRole:
            //if(isUpgrading())return QColor(Qt::white);
            //return col==FACT_MODEL_COLUMN_NAME?QColor(Qt::darkYellow):QColor(Qt::darkGray);
            //if(!statsShowTimer.isActive()) return isUpgradable()?QColor(Qt::red).lighter():Qt::darkGray;
            //nstats
            //return statsWarn?QColor(Qt::yellow):QColor(Qt::green);
            break;
        case Qt::BackgroundRole:
            //if(isUpgrading())return QColor(0x20,0x00,0x00);
            //if(isUpgradePending())return QColor(0x40,0x00,0x00);
            if (ident().flags.bits.reconf)
                return QColor(Qt::darkGray).darker(200);
            return QColor(0x20, 0x40, 0x60);
        }
    }
    if (role == Qt::FontRole && col == Fact::FACT_MODEL_COLUMN_DESCR) {
#ifdef Q_OS_MAC
        return QFont("Menlo");
#else
        return QFont("FreeMono");
#endif
    }
    return ProtocolViewBase::data(col, role);
}

void NodeItem::hashData(QCryptographicHash *h) const
{
    Fact::hashData(h);
    h->addData(version().toUtf8());
    h->addData(hardware().toUtf8());
    h->addData(QString::number(ident().hash).toUtf8());
}

//=============================================================================
// Properties
//=============================================================================

QString NodeItem::sn() const
{
    return protocol()->sn();
}
QString NodeItem::version() const
{
    return protocol()->version();
}
QString NodeItem::hardware() const
{
    return protocol()->hardware();
}
const xbus::node::ident::ident_s &NodeItem::ident() const
{
    return protocol()->ident();
}
bool NodeItem::identValid() const
{
    return m_identValid;
}
void NodeItem::setIdentValid(const bool &v)
{
    if (m_identValid == v)
        return;
    m_identValid = v;
    emit identValidChanged();
}
bool NodeItem::dictValid() const
{
    return m_dictValid;
}
void NodeItem::setDictValid(const bool &v)
{
    if (m_dictValid == v)
        return;
    m_dictValid = v;
    emit dictValidChanged();
}
bool NodeItem::dataValid() const
{
    return m_dataValid;
}
void NodeItem::setDataValid(const bool &v)
{
    if (m_dataValid == v)
        return;
    m_dataValid = v;
    emit dataValidChanged();
}
bool NodeItem::upgrading() const
{
    return m_upgrading;
}
void NodeItem::setUpgrading(const bool &v)
{
    if (m_upgrading == v)
        return;
    m_upgrading = v;
    emit upgradingChanged();
}

//=============================================================================
// Protocols connection
//=============================================================================

void NodeItem::identReceived()
{
    //qDebug() << "ok";
    m_lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

    setIdentValid(true);

    if (upgrading() || ident().flags.bits.files == 0) {
        // node in some service mode
        setDictValid(true);
        return;
    }
    if (!protocol()->enabled())
        return;

    //nodes->storage->saveNodeInfo(this);
    //nodes->storage->saveNodeUser(this);

    // node has dict
}

void NodeItem::dictReceived(const ProtocolNode::Dictionary &dict) {}

void NodeItem::confReceived(const QVariantList &values) {}

void NodeItem::messageReceived(xbus::node::msg::type_t type, QString msg)
{
    AppNotify::instance()->report(msg, AppNotify::FromVehicle | AppNotify::Important);
}
