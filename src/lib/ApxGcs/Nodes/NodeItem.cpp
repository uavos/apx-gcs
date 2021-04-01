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

#include <App/AppGcs.h>
#include <Vehicles/VehicleWarnings.h>
#include <Vehicles/Vehicles.h>
#include <QFontDatabase>

NodeItem::NodeItem(Fact *parent, Nodes *nodes, PNode *protocol)
    : Fact(parent, protocol->name())
    , _nodes(nodes)
    , _protocol(protocol)
{
    setIcon("sitemap");

    bindProperty(protocol, "title", true);
    bindProperty(protocol, "descr", true);
    //bindProperty(protocol, "value", true);
    bindProperty(protocol, "progress", true);

    setOptions(ProgressTrack | ModifiedGroup);

    qmlRegisterUncreatableType<NodeItem>("APX.Node", 1, 0, "Node", "Reference only");

    new NodeViewActions(this, _nodes);

    tools = new NodeTools(this, Action);

    //protocol
    connect(this, &Fact::removed, protocol, &Fact::deleteFact);

    connect(protocol, &PNode::messageReceived, this, &NodeItem::messageReceived);
    connect(protocol, &PNode::identReceived, this, &NodeItem::identReceived);
    connect(protocol, &PNode::dictReceived, this, &NodeItem::dictReceived);
    connect(protocol, &PNode::confReceived, this, &NodeItem::confReceived);
    connect(protocol, &PNode::confUpdated, this, &NodeItem::confUpdated);
    connect(protocol, &PNode::confSaved, this, &NodeItem::confSaved);

    connect(this, &NodeItem::shell, protocol, &PNode::requestShell);

    /*
    // responses mapping
    connect(protocol, &ProtocolNode::identReceived, this, &NodeItem::identReceived);
    connect(protocol, &ProtocolNode::dictReceived, this, &NodeItem::dictReceived);
    connect(protocol, &ProtocolNode::confReceived, this, &NodeItem::confReceived);
    connect(protocol, &ProtocolNode::confDefault, this, &NodeItem::restoreDefaults);
    connect(protocol, &ProtocolNode::messageReceived, this, &NodeItem::messageReceived);
    connect(protocol, &ProtocolNode::statusReceived, this, &NodeItem::statusReceived);


    statusTimer.setSingleShot(true);
    statusTimer.setInterval(10000);
    connect(&statusTimer, &QTimer::timeout, this, &NodeItem::updateDescr);
    connect(protocol, &ProtocolNode::identReceived, this, &NodeItem::updateDescr);*/

    protocol->requestIdent();
}

void NodeItem::validateData()
{
    if (m_valid)
        return;
    backup();
    m_valid = true;
    emit validChanged();

    updateStatus();
    qDebug() << "Node data valid:" << path();
}

void NodeItem::updateDescr()
{
    /*statusTimer.stop();
    setActive(false); // set by status
    QString s = protocol()->text();
    if (s.isEmpty())
        s = protocol()->descr();
    setDescr(s);*/
}
void NodeItem::updateStatus()
{
    if (_ident.value("reconf").toBool()) {
        setValue(tr("no config").toUpper());
        return;
    }
    if (_status_field) {
        setValue(_status_field->valueText().trimmed());
    }
}

void NodeItem::clear()
{
    if (m_valid) {
        m_valid = false;
        emit validChanged();
    }

    _status_field = nullptr;
    tools->clearCommands();
    m_fields.clear();
    deleteChildren();
    setModified(false);
}

void NodeItem::upload()
{
    if (!valid())
        return;
    if (!modified())
        return;

    QList<NodeField *> fields;
    for (auto i : m_fields) {
        if (!i->modified())
            continue;
        if (i->size() > 0) {
            for (auto j : i->facts()) {
                if (!j->modified())
                    continue;
                fields.append(static_cast<NodeField *>(j));
            }
            continue;
        }
        fields.append(static_cast<NodeField *>(i));
    }

    QVariantMap values;
    for (auto i : fields) {
        values.insert(i->fpath(), i->confValue());
        //_nodes->vehicle->recordConfigUpdate(title(), i->fpath(), i->valueText(), protocol()->sn());
    }
    _protocol->requestUpdate(values);
}
void NodeItem::confSaved()
{
    qDebug() << modified();
    backup();
}

QVariant NodeItem::data(int col, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (col == FACT_MODEL_COLUMN_NAME)
            return title().toUpper();
        break;
    case Qt::ForegroundRole:
        if (!valid()) {
            return QColor(255, 200, 200);
        }
        if (col == FACT_MODEL_COLUMN_DESCR)
            return QColor(Qt::darkGray);
        if (col == FACT_MODEL_COLUMN_VALUE)
            return QColor(Qt::yellow).lighter(180);
        if (!modified())
            return QColor(255, 255, 200);
        break;
        //break;
    case Qt::BackgroundRole:
        if (valid()) {
            return QColor(0x10, 0x20, 0x30);
        } else {
            return QVariant();
        }
        // if (!protocol()->valid())
        //     return QVariant();
        // if (protocol()->ident().flags.bits.reconf)
        //     return QColor(Qt::darkGray).darker(200);
        // return QColor(0x20, 0x40, 0x60);
    case Qt::FontRole: {
#ifdef Q_OS_MAC
        QFont font("Menlo");
#else
        QFont font("FreeMono");
#endif
        if (col == Fact::FACT_MODEL_COLUMN_DESCR)
            return font;
        if (col == FACT_MODEL_COLUMN_NAME) {
            font.setBold(true);
            return font;
        }
    } break;
    }
    return Fact::data(col, role);
}
QString NodeItem::toolTip() const
{
    QStringList st;
    st << "ident:";
    st.append(QJsonDocument::fromVariant(_ident).toJson());
    return Fact::toolTip().append("\n").append(st.join('\n'));
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

    bool bControls = group->name() == "controls";

    int colCnt = group->size();
    for (int row = 0; row < f1->size(); ++row) {
        Fact *fi = f1->child(row);
        Fact *fRow = new Fact(action, fi->name(), fi->title(), "", Group | ModifiedGroup);
        new NodeViewActions(fRow, _nodes);

        fRow->bindProperty(fi, "valueText", true);
        fRow->bindProperty(fi, "modified", true);

        connect(fi, &Fact::valueTextChanged, this, [this, fRow]() { updateArrayRowDescr(fRow); });

        Fact *f_ch = nullptr;
        int f_ch_max = 0;
        for (int i = 0; i < colCnt; ++i) {
            Fact *fArray = group->child(i);
            if (!fArray)
                continue;
            Fact *fp;
            bool bChParam = false;
            if (bControls && f_ch && fArray->name().startsWith("ch_")) {
                fp = fArray->child(0);
                f_ch_max = fArray->size();
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
            connect(f, &Fact::valueTextChanged, fRow, [this, fRow]() { updateArrayRowDescr(fRow); });
            if (bChParam) {
                updateArrayChBinding(f, fArray, f_ch);
                connect(f_ch, &Fact::valueChanged, f, [this, f, fArray, f_ch]() {
                    updateArrayChBinding(f, fArray, f_ch);
                });
            } else {
                f->setBinding(fp);
            }

            if (bControls && f->name() == "ch") {
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
            st.append(i->valueText());
        }
    }
    fRow->setDescr(st.join(", "));
}
void NodeItem::updateArrayChBinding(Fact *f_element, Fact *f_array, Fact *f_ch)
{
    int ch = f_ch->value().toInt();
    f_element->setBinding(nullptr);
    if (ch > 0 && ch <= f_array->size()) {
        f_element->setEnabled(true);
        f_element->setBinding(f_array->child(ch - 1));
    } else {
        f_element->setValue(0);
        f_element->setValueText("");
        f_element->setModified(false);
        f_element->setEnabled(false);
    }
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
        f->deleteFact();
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
        connect(f, &Fact::valueTextChanged, f->parentFact(), [f]() {
            f->parentFact()->setValue(f->valueText());
        });
        f->parentFact()->setValue(f->valueText());
        return;
    }

    for (auto i : f->facts()) {
        linkGroupValues(i);
    }
}

//=============================================================================
// Protocols connection
//=============================================================================

void NodeItem::identReceived(QVariantMap ident)
{
    if (_ident == ident)
        return;

    qWarning() << "ident updated";
    _ident = ident;
    clear();

    _protocol->requestDict();
}

void NodeItem::dictReceived(QVariantList dict)
{
    clear();

    xbus::node::usr::cmd_t cmd_cnt = 0;
    for (auto const &i : dict) {
        auto field = i.value<QVariantMap>();
        QString name = field.value("name").toString();
        QString title = field.value("title").toString();
        QString type = field.value("type").toString();

        // find group fact
        Fact *g = this;
        QStringList path = name.split('.');
        while (path.size() > 1) {
            g = g->child(path.takeFirst());
            if (g)
                continue;
            qWarning() << "missing group" << name;
            g = this;
            break;
        }
        name = path.first();

        if (type == "group") {
            g = new Fact(g, name, title, "", Group | ModifiedGroup);
            new NodeViewActions(g, _nodes);
        } else if (type == "command") {
            tools->addCommand(g, name, title, cmd_cnt++);
        } else { // data field
            NodeField *f = new NodeField(g, this, field, m_fields.size());
            m_fields.append(f);
            if (!_status_field) {
                if (type == "string") {
                    _status_field = f;
                    connect(_status_field, &Fact::valueChanged, this, &NodeItem::updateStatus);
                }
            }
        }
    }
    //qDebug() << m_fields.size();
    removeEmptyGroups(this);
    groupArrays();
    linkGroupValues(this);

    // update descr and help from APXFW package
    _parameters = AppGcs::apxfw()->loadParameters(title(), _ident.value("hardware").toString());
    for (auto v : _parameters) {
        updateMetadataAPXFW(this, this, v);
    }
    //setEnabled(false);
    backup();

    _protocol->requestConf();
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

void NodeItem::confReceived(QVariantMap values)
{
    if (m_fields.isEmpty()) {
        qWarning() << "missing dict:" << title();
        return;
    }

    QStringList fields;
    for (auto f : m_fields) {
        QString fpath = f->fpath();
        if (!values.contains(fpath)) {
            qWarning() << "missing data for:" << fpath;
            continue;
        }
        fields.append(fpath);
        f->setConfValue(values.value(fpath));
    }

    if (valid())
        return;

    // report missing fields
    for (auto fpath : values.keys()) {
        if (!fields.contains(fpath)) {
            qWarning() << "missing field for:" << fpath;
        }
    }
    if (fields.size() != m_fields.size()) {
        apxMsgW() << tr("Inconsistent parameters");
        return;
    }

    validateData();
    setEnabled(true);
}
void NodeItem::confUpdated(QVariantMap values)
{
    if (!valid())
        return;
    for (auto f : m_fields) {
        QString fpath = f->fpath();
        if (!values.contains(fpath))
            continue;
        f->setConfValue(values.value(fpath));
        apxMsgW() << tr("Field modified").append(':') << fpath.append(':') << f->text();
    }
}

void NodeItem::messageReceived(PNode::msg_type_e type, QString msg)
{
    AppNotify::NotifyFlags flags = AppNotify::FromVehicle | AppNotify::Important;
    switch (type) {
    default:
        break;
    case PNode::warn:
        flags |= AppNotify::Warning;
        break;
    case PNode::err:
        flags |= AppNotify::Error;
        break;
    }
    message(msg, flags);
}
void NodeItem::message(QString msg, AppNotify::NotifyFlags flags)
{
    QString s = title();
    if (!valueText().isEmpty()) {
        s.append(QString("/%1").arg(valueText()));
    }
    _nodes->vehicle->message(msg, flags, s);
    //_nodes->vehicle->recordNodeMessage(s, msg, protocol()->sn());
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
