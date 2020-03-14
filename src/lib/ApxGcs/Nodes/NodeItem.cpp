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
    setOptions(ProgressTrack);
    qmlRegisterUncreatableType<NodeItem>("APX.Node", 1, 0, "Node", "Reference only");

    //tools = new NodeTools(this, Action);

    //protocol
    connect(protocol, &QObject::destroyed, this, [this]() { deleteLater(); });

    // validity
    connect(protocol, &ProtocolNode::identChanged, this, &NodeItem::clear);
    connect(protocol, &ProtocolNode::dictValidChanged, this, &NodeItem::validateDict);
    connect(protocol, &ProtocolNode::dataValidChanged, this, &NodeItem::validateData);

    connect(protocol, &ProtocolNode::descrChanged, this, &NodeItem::updateDescr);

    // responses mapping
    connect(protocol, &ProtocolNode::identReceived, this, &NodeItem::identReceived);
    connect(protocol, &ProtocolNode::dictReceived, this, &NodeItem::dictReceived);
    connect(protocol, &ProtocolNode::confReceived, this, &NodeItem::confReceived);
    connect(protocol, &ProtocolNode::messageReceived, this, &NodeItem::messageReceived);

    //FIXME: nodes->storage->loadNodeInfo(this);

    statusTimer.setSingleShot(true);
    statusTimer.setInterval(10000);
    connect(&statusTimer, &QTimer::timeout, this, &NodeItem::updateDescr);
}

void NodeItem::validateDict()
{
    if (!protocol()->dictValid())
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
    if (!protocol()->dataValid())
        return;
    if (!protocol()->enabled())
        return;
    if (protocol()->ident().flags.bits.reconf) {
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
    setDescr(protocol()->descr());
}
void NodeItem::updateStatus()
{
    if (protocol()->ident().flags.bits.reconf) {
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
    setModified(false);
}

void NodeItem::upload()
{
    if (!protocol()->dataValid())
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
    switch (role) {
    case Qt::ForegroundRole:
        if (protocol()->dataValid()) {
            if (col == FACT_MODEL_COLUMN_DESCR)
                return QColor(Qt::darkGray);
            if (col == FACT_MODEL_COLUMN_VALUE)
                return QColor(Qt::yellow).lighter(180);
        }
        if (protocol()->upgrading())
            return QColor(255, 200, 200);
        if (!protocol()->dictValid())
            return QColor(255, 200, 200);
        if (!protocol()->dataValid())
            return col == FACT_MODEL_COLUMN_NAME ? QColor(255, 255, 200) : QColor(Qt::darkGray);
        break;
    case Qt::BackgroundRole:
        if (protocol()->dataValid()) {
            return QColor(0x10, 0x20, 0x30);
        }
        if (protocol()->upgrading())
            return QColor(64, 0, 0);
        if (!protocol()->dictValid())
            return QVariant();
        if (!protocol()->dataValid())
            return QVariant();
        if (protocol()->ident().flags.bits.reconf)
            return QColor(Qt::darkGray).darker(200);
        return QColor(0x20, 0x40, 0x60);
    case Qt::FontRole:
        if (col == Fact::FACT_MODEL_COLUMN_DESCR) {
#ifdef Q_OS_MAC
            return QFont("Menlo");
#else
            return QFont("FreeMono");
#endif
        }
        break;
    }
    return ProtocolViewBase::data(col, role);
}

void NodeItem::hashData(QCryptographicHash *h) const
{
    Fact::hashData(h);
    h->addData(protocol()->version().toUtf8());
    h->addData(protocol()->hardware().toUtf8());
    h->addData(QString::number(protocol()->ident().hash).toUtf8());
}

//=============================================================================
// Protocols connection
//=============================================================================

void NodeItem::identReceived()
{
    m_lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

    protocol()->setDictValid(true);
    protocol()->setDataValid(true);
}

void NodeItem::dictReceived(const ProtocolNode::Dictionary &dict) {}

void NodeItem::confReceived(const QVariantList &values) {}

void NodeItem::messageReceived(xbus::node::msg::type_t type, QString msg)
{
    AppNotify::instance()->report(msg, AppNotify::FromVehicle | AppNotify::Important);
}
