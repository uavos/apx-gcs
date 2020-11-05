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
#include "NodeItem.h"
#include "NodeField.h"
#include "NodeTools.h"
#include "NodeViewActions.h"
#include "Nodes.h"
#include <QtSql>

#include <App/AppGcs.h>
#include <Vehicles/VehicleWarnings.h>
#include <Vehicles/Vehicles.h>
#include <QFontDatabase>
#include <QQmlEngine>

NodeItem::NodeItem(Fact *parent, Nodes *nodes, ProtocolNode *protocol)
    : ProtocolViewBase(parent, protocol)
    , _nodes(nodes)
{
    setOptions(ProgressTrack | ModifiedGroup);
    qmlRegisterUncreatableType<NodeItem>("APX.Node", 1, 0, "Node", "Reference only");

    unbindProperty("value"); //unbind from protocol

    new NodeViewActions(this, _nodes);

    tools = new NodeTools(this, Action);

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
    connect(protocol, &ProtocolNode::confDefault, this, &NodeItem::restoreDefaults);
    connect(protocol, &ProtocolNode::messageReceived, this, &NodeItem::messageReceived);
    connect(protocol, &ProtocolNode::statusReceived, this, &NodeItem::statusReceived);

    connect(this, &NodeItem::shell, protocol, &ProtocolNode::requestMod);

    statusTimer.setSingleShot(true);
    statusTimer.setInterval(10000);
    connect(&statusTimer, &QTimer::timeout, this, &NodeItem::updateDescr);
    connect(protocol, &ProtocolNode::identReceived, this, &NodeItem::updateDescr);
}

const QList<NodeField *> &NodeItem::fields() const
{
    return m_fields;
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
    tools->clearCommands();
    m_fields.clear();
    removeAll();
    setModified(false);
}

void NodeItem::upload()
{
    if (!protocol()->valid())
        return;
    if (!modified())
        return;

    QVariantMap values;
    for (auto i : m_fields) {
        if (!i->modified())
            continue;
        _nodes->vehicle->recordConfigUpdate(title(), i->fpath(), i->text(), protocol()->sn());
        values.insert(i->fpath(), i->confValue());
    }
    protocol()->requestUpdate(values);
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
    //return;
    //hide grouped arrays (gpio, controls etc)
    FactList groups;
    for (auto f : m_fields) {
        Fact *groupItem = f->parentFact();
        if (!groupItem || groupItem == this || groups.contains(groupItem))
            continue;

        if (groupItem && groupItem->size() > 1) {
            //qDebug() << groupItem->path();
            //check if group members are arrays
            bool bArray = false;
            uint cnt = 0;
            for (auto i : groupItem->facts()) {
                bArray = false;
                NodeField *f = qobject_cast<NodeField *>(i);
                if (!f) {
                    break;
                }
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
    //create action with underlaying table structure to edit array rows
    Fact *action = new Fact(group, group->name(), group->title(), group->descr(), Action);
    group->setMenu(action);
    //action->setActionsModel(group->actionsModel());
    new NodeViewActions(action, _nodes);
    action->bindProperty(group, "modified", true);

    //hide group members
    for (auto i : group->facts()) {
        i->setVisible(false);
    }

    Fact *f1 = group->child(0);

    group->bindProperty(f1, "value", true);

    int colCnt = group->size();
    for (int row = 0; row < f1->size(); ++row) {
        Fact *fi = f1->child(row);
        Fact *fRow = new Fact(action, fi->name(), fi->title(), "", Group | ModifiedGroup);
        new NodeViewActions(fRow, _nodes);

        fRow->bindProperty(fi, "text", true);
        fRow->bindProperty(fi, "modified", true);

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
                               fp->treeType() | fp->dataType() | ModifiedTrack);
            new NodeViewActions(f, _nodes);
            f->setBinding(fp);
            connect(f, &Fact::textChanged, fRow, [this, fRow]() { updateArrayRowDescr(fRow); });
            if (bChParam) {
                connect(f_ch, &Fact::valueChanged, f, [f, fArray, f_ch]() {
                    f->setBinding(fArray->child(f_ch->value().toInt()));
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
    if (!fRow->isZero()) {
        for (auto i : fRow->facts()) {
            st.append(i->text());
        }
    }
    fRow->setDescr(st.join(", "));
}

void NodeItem::removeEmptyGroups(Fact *f)
{
    if (qobject_cast<NodeField *>(f))
        return;

    for (auto i : f->facts()) {
        removeEmptyGroups(i);
    }
    if (f != this && f->size() == 0) {
        //qDebug() << f;
        f->remove();
    }
}

void NodeItem::linkGroupValues(Fact *f)
{
    if (f->parentFact() != this && qobject_cast<NodeField *>(f)) {
        if (qobject_cast<NodeField *>(f->parentFact()))
            return;
        do {
            if (f->num() != 0)
                return;
            if (f->dataType() == Text)
                break;
            if (f->dataType() == Enum
                && (f->title() == "mode" || f->title() == "type" || f->title() == "enable"))
                break;
            return;
        } while (0);
        connect(f, &Fact::textChanged, f->parentFact(), [f]() {
            f->parentFact()->setValue(f->text());
        });
        return;
    }

    for (auto i : f->facts()) {
        linkGroupValues(i);
    }
}

//=============================================================================
// Protocols connection
//=============================================================================

void NodeItem::identReceived()
{
    updateStatus();
}

void NodeItem::dictReceived(const ProtocolNode::Dict &dict)
{
    //qDebug() << dict.size();

    clear();

    QMap<int, Fact *> groups;
    xbus::node::usr::cmd_t cmd_cnt = 0;
    Fact *g = this;
    NodeField *f;
    for (auto const &i : dict) {
        switch (i.type) {
        case xbus::node::conf::group:
            g = i.group ? groups.value(i.group) : this;
            g = new Fact(g, i.name, i.title, "", Group | ModifiedGroup);
            new NodeViewActions(g, _nodes);
            groups.insert(groups.size() + 1, g);
            break;
        case xbus::node::conf::command:
            g = i.group ? groups.value(i.group) : this;
            tools->addCommand(g, i.name, i.title, cmd_cnt++);
            break;
        default: // data field
            g = i.group ? groups.value(i.group) : this;
            f = new NodeField(g, this, static_cast<xbus::node::conf::fid_t>(m_fields.size()), i);
            f->setEnabled(false);
            m_fields.append(f);
            if (!m_status_field) {
                if (i.type == xbus::node::conf::string) {
                    m_status_field = f;
                    connect(m_status_field, &Fact::valueChanged, this, &NodeItem::updateStatus);
                }
            }
        }
    }
    //qDebug() << m_fields.size();
    removeEmptyGroups(this);
    groupArrays();
    linkGroupValues(this);

    // update descr and help from APXFW package
    _parameters = AppGcs::apxfw()->loadParameters(title(), protocol()->hardware());
    for (auto v : _parameters) {
        updateMetadataAPXFW(this, this, v);
    }
    setEnabled(false);
    backup();
}
static QVariant jsonToVariant(QJsonValue json)
{
    switch (json.type()) {
    default:
        break;
    case QJsonValue::Double:
        return QString::number(json.toVariant().toFloat());
    case QJsonValue::Array: {
        QVariantList list;
        for (auto v : json.toArray())
            list.append(jsonToVariant(v));
        return list;
    }
    }
    return json.toVariant();
}

void NodeItem::updateMetadataAPXFW(Fact *root, Fact *group, QJsonValue json)
{
    if (!json.isObject())
        return;
    QJsonObject obj = json.toObject();
    if (!obj.contains("name"))
        return;
    QString name = obj["name"].toString();
    Fact *fx = group->child(name, Qt::CaseSensitive);
    if (!fx)
        return;

    QString title = obj["title"].toString();
    QString descr = obj["descr"].toString();

    NodeField *nf = qobject_cast<NodeField *>(fx);
    if (nf) {
        nf->setHelp(descr);
        descr = title;
    }
    fx->setDescr(descr);

    // default values
    if (obj.contains("default")) {
        QJsonValue def = obj["default"];
        if (nf) {
            fx->setDefaultValue(jsonToVariant(def));
        } else if (def.isObject()) {
            QJsonObject def_obj = def.toObject();
            for (auto key : def_obj.keys()) {
                Fact *f = fx->findChild(key);
                if (!f) {
                    qWarning() << "Unsupported defaults object format" << def << fx->path();
                    continue;
                }
                f->setDefaultValue(jsonToVariant(def_obj[key]));
            }
        } else {
            qWarning() << "Unsupported defaults format" << def << fx->path();
        }
    }

    // parse child objects
    if (!obj.contains("content"))
        return;
    for (auto v : obj["content"].toArray()) {
        updateMetadataAPXFW(root, fx, v);
    }
}

bool NodeItem::loadConfigValue(const QString &name, const QString &value)
{
    for (auto f : m_fields) {
        if (f->fpath() != name)
            continue;
        f->setConfValue(value);
        return true;
    }
    return false;
}

void NodeItem::confReceived(const QVariantMap &values)
{
    //qDebug() << values;
    setEnabled(true);
    for (auto f : m_fields) {
        if (!values.contains(f->fpath()))
            continue;
        f->setConfValue(values.value(f->fpath()));
        f->setEnabled(true);
    }
    if (!protocol()->valid()) {
        backup();
    }

    updateStatus();
}

void NodeItem::messageReceived(xbus::node::msg::type_e type, QString msg)
{
    AppNotify::NotifyFlags flags = AppNotify::FromVehicle | AppNotify::Important;
    switch (type) {
    default:
        break;
    case xbus::node::msg::warn:
        flags |= AppNotify::Warning;
        break;
    case xbus::node::msg::err:
        flags |= AppNotify::Error;
        break;
    }
    message(msg, flags);
}
void NodeItem::message(QString msg, AppNotify::NotifyFlags flags)
{
    QString s = title();
    if (!text().isEmpty()) {
        s.append(QString("/%1").arg(text()));
    }
    _nodes->vehicle->message(msg, flags, s);
    _nodes->vehicle->recordNodeMessage(s, msg, protocol()->sn());
}
void NodeItem::statusReceived(const xbus::node::status::status_s &status)
{
    statusTimer.start();
    setActive(true);
    QStringList st;
    st << QString("H:%1").arg(status.err.hw, 2, 16, QChar('0')).toUpper();
    st << QString("C:%1").arg(status.err.can, 2, 16, QChar('0')).toUpper();
    st << QString("U:%1").arg(status.err.uart, 2, 16, QChar('0')).toUpper();
    st << QString("RT:%1/%2")
              .arg(status.cnt.rx, 2, 16, QChar('0'))
              .toUpper()
              .arg(status.cnt.tx, 2, 16, QChar('0'))
              .toUpper();
    setDescr(st.join(' '));
}
