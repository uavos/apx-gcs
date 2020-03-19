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

    setValue(QVariant()); //unbind from protocol

    //FIXME: tools = new NodeTools(this, Action);

    //protocol
    connect(protocol, &QObject::destroyed, this, [this]() { deleteLater(); });

    // validity
    connect(protocol, &ProtocolNode::identChanged, this, &NodeItem::clear);
    connect(protocol, &ProtocolNode::dictValidChanged, this, &NodeItem::validateDict);
    connect(protocol, &ProtocolNode::validChanged, this, &NodeItem::validateData);

    connect(protocol, &ProtocolNode::descrChanged, this, &NodeItem::updateDescr);
    connect(protocol, &ProtocolNode::valueChanged, this, &NodeItem::updateDescr);

    // responses mapping
    connect(protocol, &ProtocolNode::identReceived, this, &NodeItem::identReceived);
    connect(protocol, &ProtocolNode::dictReceived, this, &NodeItem::dictReceived);
    connect(protocol, &ProtocolNode::confReceived, this, &NodeItem::confReceived);
    connect(protocol, &ProtocolNode::confSaved, this, &NodeItem::confSaved);
    connect(protocol, &ProtocolNode::messageReceived, this, &NodeItem::messageReceived);

    //FIXME: nodes->storage->loadNodeInfo(this);

    statusTimer.setSingleShot(true);
    statusTimer.setInterval(10000);
    connect(&statusTimer, &QTimer::timeout, this, &NodeItem::updateDescr);
}

void NodeItem::validateDict()
{
    if (!protocol()->dictValid()) {
        clear();
        return;
    }
    //groupFields();
    //qDebug()<path();

    //fields map
    /*for (int i = 0; i < allFields.size(); ++i) {
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
    }*/
}
void NodeItem::validateData()
{
    if (!protocol()->valid())
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
    setActive(false); // set by status
    QString s = protocol()->text();
    if (s.isEmpty())
        s = protocol()->descr();
    setDescr(s);
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
    if (!protocol()->valid())
        return;
    if (!modified())
        return;
    //saveTelemetryUploadEvent();
    int cnt = 0;
    foreach (NodeField *f, allFields) {
        if (!f->modified())
            continue;
        protocol()->requestUpdate(f->fid(), f->uploadableValue());
        cnt++;
    }
    if (cnt > 0) {
        protocol()->requestUpdateSave();
    }
}
void NodeItem::confSaved()
{
    backup();
}

QVariant NodeItem::data(int col, int role) const
{
    switch (role) {
    case Qt::ForegroundRole:
        if (protocol()->valid()) {
            if (col == FACT_MODEL_COLUMN_DESCR)
                return QColor(Qt::darkGray);
            if (col == FACT_MODEL_COLUMN_VALUE)
                return QColor(Qt::yellow).lighter(180);
        }
        if (!protocol()->dictValid())
            return QColor(255, 200, 200);
        if (!protocol()->valid())
            return col == FACT_MODEL_COLUMN_NAME ? QColor(255, 255, 200) : QColor(Qt::darkGray);
        break;
    case Qt::BackgroundRole:
        if (protocol()->valid()) {
            return QColor(0x10, 0x20, 0x30);
        }
        if (!protocol()->dictValid())
            return QVariant();
        if (!protocol()->valid())
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
    return Fact::data(col, role);
}

void NodeItem::groupArrays()
{
    //hide grouped arrays (gpio, controls etc)
    QList<Fact *> groups;
    for (auto f : allFields) {
        Fact *groupItem = f->parentFact();
        if (!groupItem || groupItem == this || groups.contains(groupItem))
            continue;

        if (groupItem && groupItem->size() > 1) {
            //qDebug() << groupItem->path();
            //check if group members are arrays
            bool bArray = false;
            uint cnt = 0;
            for (int i = 0; i < groupItem->size(); ++i) {
                NodeField *f = groupItem->child<NodeField>(i);
                bArray = false;
                if (cnt < 2 && f->size() == 0)
                    break;
                cnt++;
                bArray = true;
            }
            if (bArray)
                groups.append(groupItem);
        }
    }
    for (auto group : groups) {
        //qDebug() << group->path();
        groupArrays(group);
    }
}

void NodeItem::groupArrays(Fact *group)
{
    //create action with underlaying table structure to edit arrays rows

    Fact *action = new Fact(group, group->name(), group->title(), group->descr(), Action);
    //group->bind(action);
    group->setModel(action->model());

    connect(group->child(0), &Fact::valueChanged, group, [group]() {
        group->setValue(group->child(0)->value());
    });
    group->setValue(group->child(0)->value());

    //hide group members
    for (auto i : *group) {
        //static_cast<Fact *>(i)->setVisible(false);
    }

    Fact *f1 = group->child(0);
    int colCnt = group->size();
    for (int row = 0; row < f1->size(); ++row) {
        Fact *fi = f1->child(row);
        Fact *fRow = new Fact(action, fi->name(), fi->title(), "", Group);
        /*connect(
            group,
            &Fact::modifiedChanged,
            fRow,
            [fRow, group]() { fRow->setModified(group->modified()); },
            Qt::QueuedConnection);*/
        connect(fi, &Fact::valueChanged, fRow, [fRow, fi]() { fRow->setValue(fi->value()); });
        connect(fi, &Fact::textChanged, this, [this, fRow]() { updateArrayRowDescr(fRow); });

        Fact *f_ch = nullptr;
        int f_ch_max = 0;
        for (int i = 0; i < colCnt; ++i) {
            Fact *fArray = group->child(i);
            if (!fArray)
                continue;
            Fact *fp;
            bool bChParam = false;
            if (f_ch && fArray->name().contains("_controls_ch_")) {
                fp = fArray->child(f_ch->value().toInt());
                f_ch_max = fArray->size() - 1;
                bChParam = true;
            } else {
                fp = fArray->child(row);
            }
            if (!fp)
                continue;
            Fact *f = new Fact(fRow,
                               fArray->name(),
                               fArray->title(),
                               fArray->descr(),
                               fp->treeType() | fp->dataType());
            f->bind(fp);
            connect(f, &Fact::textChanged, fRow, [this, fRow]() { updateArrayRowDescr(fRow); });
            if (bChParam) {
                connect(f_ch, &Fact::valueChanged, f, [f, fArray, f_ch]() {
                    f->bind(fArray->child(f_ch->value().toInt()));
                });
            }

            if (f->name().endsWith("_controls_ch")) {
                f_ch = f;
            }
        }
        if (f_ch && f_ch_max > 0) {
            f_ch->setMax(f_ch_max);
        }
    }
}
void NodeItem::updateArrayRowDescr(Fact *fRow)
{
    QStringList st;
    if (!fRow->value().toString().isEmpty()) {
        for (int i = 0; i < fRow->size(); ++i) {
            st.append(fRow->child(i)->text());
        }
    }
    fRow->setDescr(st.join(", "));
}

//=============================================================================
// Protocols connection
//=============================================================================

void NodeItem::identReceived()
{
    m_lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (!protocol()->dictValid()) {
        protocol()->requestDict();
    }
}

void NodeItem::dictReceived(const ProtocolNode::Dict &dict)
{
    //qDebug() << dict.size();

    allFields.clear();
    allFieldsByName.clear();
    m_status_field = nullptr;

    QMap<int, Fact *> groups;
    Fact *g = this;
    NodeField *f;
    for (auto const &i : dict) {
        switch (i.type) {
        case xbus::node::conf::group:
            g = i.group ? groups.value(i.group) : this;
            g = new Fact(g, i.name, i.title, i.descr, Group);
            groups.insert(groups.size() + 1, g);
            break;
        case xbus::node::conf::command:
            break;
        default: // data field
            g = i.group ? groups.value(i.group) : this;
            f = new NodeField(g, this, static_cast<xbus::node::conf::fid_t>(allFields.size()), i);
            f->setEnabled(false);
            allFields.append(f);
            allFieldsByName.insert(i.name, f);
            if (!m_status_field) {
                if (i.type == xbus::node::conf::string) {
                    m_status_field = f;
                    connect(m_status_field, &Fact::valueChanged, this, &NodeItem::updateStatus);
                }
            }
        }
    }
    groupArrays();
}

void NodeItem::confReceived(const QVariantList &values)
{
    //qDebug() << values;
    if (values.size() != allFields.size()) {
        qWarning() << "fields mismatch:" << values.size() << allFields.size();
        protocol()->setValid(false);
        return;
    }
    int i = 0;
    for (auto f : allFields) {
        f->setValue(values.at(i++));
        f->setEnabled(true);
    }
    backup();
    updateStatus();
}

void NodeItem::messageReceived(xbus::node::msg::type_t type, QString msg)
{
    AppNotify::instance()->report(msg, AppNotify::FromVehicle | AppNotify::Important);
}
