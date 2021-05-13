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
    : Fact(parent, "node#")
    , _nodes(nodes)
    , _protocol(protocol)
{
    setIcon("sitemap");

    setOptions(ProgressTrack | ModifiedGroup);

    qmlRegisterUncreatableType<NodeItem>("APX.Node", 1, 0, "Node", "Reference only");

    new NodeViewActions(this, _nodes);

    storage = new NodeStorage(this);
    tools = new NodeTools(this, Action);

    //protocol
    if (protocol) {
        bindProperty(protocol, "title");
        bindProperty(protocol, "descr", true);
        bindProperty(protocol, "progress", true);

        connect(this, &Fact::removed, protocol, &Fact::deleteFact);

        connect(protocol, &PNode::messageReceived, this, &NodeItem::messageReceived);
        connect(protocol, &PNode::identReceived, this, &NodeItem::identReceived);
        connect(protocol, &PNode::dictReceived, this, &NodeItem::dictReceived);
        connect(protocol, &PNode::confReceived, this, &NodeItem::confReceived);
        connect(protocol, &PNode::confUpdated, this, &NodeItem::confUpdated);
        connect(protocol, &PNode::confSaved, this, &NodeItem::confSaved);

        connect(protocol, &PNode::upgradingChanged, this, &NodeItem::updateUpgrading);
        connect(protocol, &PNode::upgradingChanged, this, &NodeItem::updateStatus);

        connect(this, &NodeItem::shell, protocol, [this](QStringList commands) {
            _protocol->requestMod(PNode::sh, QByteArray(), commands);
        });

        connect(protocol, &Fact::valueChanged, this, [this]() {
            if (_protocol->value().isNull())
                setDescr(_protocol->descr());
            else
                setDescr(_protocol->value().toString());
        });
    }

    // models decorations update
    connect(this, &NodeItem::aliveChanged, this, &Fact::enabledChanged);

    if (protocol && !_nodes->upgrading() && !_nodes->vehicle->isLocal())
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

    if (_protocol)
        _nodes->nodeNotify(this);
}

void NodeItem::updateStatus()
{
    if (_protocol && _protocol->upgrading()) {
        FactData::setValue(QVariant());
        return;
    }
    if (_ident.value("reconf").toBool()) {
        setValue(tr("no config").toUpper());
        return;
    }
    setValue(label());
}
void NodeItem::updateUpgrading()
{
    if (_protocol->upgrading()) {
        //qDebug() << "UPGRADING:" << title() << _nodes->vehicle->title();
        clear();
        return;
    }
    //qDebug() << "UPGRADING FINISHED:" << title() << _nodes->vehicle->title();
    if (!_nodes->vehicle->isLocal()) {
        _protocol->requestIdent();
    }
}
void NodeItem::updateAlive(bool alive)
{
    uint v = m_alive;
    if (alive) {
        v = alive_cnt;
    } else if (m_alive > 0)
        v = m_alive - 1;

    if (m_alive == v)
        return;
    m_alive = v;
    emit aliveChanged();
}

void NodeItem::clear()
{
    if (m_valid) {
        m_valid = false;
        emit validChanged();
    }

    storage->updateConfigID(0);
    _dict.clear();
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
    if (!_protocol)
        return;

    storage->updateConfigID(0);

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
        values.insert(i->fpath(), i->toVariant());
        _nodes->fieldUploadReport(this, i->fpath(), i->valueText());
    }
    _protocol->requestUpdate(values);
}
void NodeItem::confSaved()
{
    qDebug() << modified();
    backup();
    storage->saveNodeConfig();
}

QVariant NodeItem::data(int col, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (col == FACT_MODEL_COLUMN_NAME)
            return title();
        break;
    case Qt::ForegroundRole:
        if (!valid()) {
            return QColor(255, 200, 200);
        }
        if (col == FACT_MODEL_COLUMN_DESCR)
            return QColor(Qt::darkGray);
        if (col == FACT_MODEL_COLUMN_VALUE)
            return QColor(Qt::yellow).lighter(180);
        if (!_protocol)
            return QColor(Qt::darkGray);

        if (modified())
            break;

        if (alive() == alive_cnt)
            return QColor(255, 255, 200);

        if (alive() == 0)
            return QColor(255, 200, 200).darker(200);

        return QColor(255, 255, 255).darker(100 + (alive_cnt - alive()) * 30);
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
        if (col == Fact::FACT_MODEL_COLUMN_DESCR)
            return App::getMonospaceFont();
        if (col == FACT_MODEL_COLUMN_NAME) {
            auto font = App::getMonospaceFont();
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

QVariantMap NodeItem::get_info() const
{
    QVariantMap m;
    m.insert("uid", _ident.value("uid"));
    m.insert("name", _ident.value("name"));
    m.insert("version", _ident.value("version"));
    m.insert("hardware", _ident.value("hardware"));
    m.insert("user", _ident.value("user"));
    if (_lastSeenTime)
        m.insert("time", _lastSeenTime);
    return m;
}
QVariantMap NodeItem::get_dict() const
{
    return _dict;
}
QVariantMap NodeItem::get_values() const
{
    QVariantMap m;
    for (auto f : m_fields) {
        m.insert(f->fpath(), f->toVariant());
    }
    return m;
}
QVariant NodeItem::toVariant() const
{
    QVariantMap m;
    m.insert("info", get_info());
    m.insert("dict", get_dict());
    m.insert("values", get_values());
    m.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    return m;
}
void NodeItem::fromVariant(const QVariant &var)
{
    QVariantMap m = var.value<QVariantMap>();
    if (m.isEmpty())
        return;

    auto info = m.value("info").value<QVariantMap>();
    auto dict = m.value("dict").value<QVariantMap>();
    auto values = m.value("values").value<QVariantMap>();

    if (!valid()) {
        // construct the whole node
        identReceived(info);
        dictReceived(dict);
        confReceived(values);
        return;
    }

    importValues(values);
}

void NodeItem::importValues(QVariantMap values)
{
    QStringList st = values.keys();
    for (auto f : m_fields) {
        QString fpath = f->fpath();
        if (!values.contains(fpath))
            continue;
        f->fromVariant(values.value(fpath));
        st.removeOne(fpath);
    }
    if (st.size() > 0) {
        for (auto i : st) {
            qWarning() << "missing field:" << i;
        }
        auto rcnt = values.size() - st.size();
        message(tr("Imported %1 fields of %2").arg(rcnt).arg(values.size()),
                AppNotify::FromApp | AppNotify::Warning);
    } else {
        //message(tr("Imported config"), AppNotify::FromApp);
    }
}

NodeField *NodeItem::field(QString name) const
{
    QRegExp re("_(\\d+)$");
    auto a = re.indexIn(name);
    int aidx = -1;
    if (a > 1) {
        auto i = re.cap(1).toInt() - 1;
        if (i < 0) {
            qWarning() << "array" << name;
            return nullptr;
        }
        name = name.left(a);
        aidx = i;
    }
    NodeField *f = nullptr;
    for (auto i : m_fields) {
        if (i->fpath() != name)
            continue;
        f = i;
        break;
    }
    if (!f)
        return nullptr;
    if (aidx < 0 && f->size() <= 0)
        return f;

    if (aidx >= f->size())
        return nullptr;
    return qobject_cast<NodeField *>(f->child(aidx));
}

// Protocols connection

void NodeItem::identReceived(QVariantMap ident)
{
    if (ident.isEmpty())
        return;

    if (!ident.contains("user")) {
        QVariantMap user;
        user.insert("machineUID", App::machineUID());
        user.insert("username", App::username());
        user.insert("hostname", App::hostname());
        ident.insert("user", user);
    }

    if (_ident == ident && valid())
        return;

    qWarning() << "ident updated" << title() << _nodes->vehicle->title();

    _ident = ident;
    clear();

    if (_protocol && !_protocol->upgrading()) {
        _lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        storage->saveNodeInfo();
        _nodes->nodeNotify(this);

        // try to request dict automatically
        do {
            if (_nodes->vehicle->isLocal() && !_nodes->vehicle->active())
                break;
            if (_nodes->upgrading())
                break;

            _protocol->requestDict();
        } while (0);
        return;
    }

    setTitle(_ident.value("name").toString());
    QStringList descr;
    descr.append(_ident.value("hardware").toString());
    descr.append(_ident.value("version").toString());
    descr.append("OFFLINE");
    setDescr(descr.join(' '));
}

void NodeItem::dictReceived(QVariantMap dict)
{
    if (dict.isEmpty())
        return;

    if (valid())
        return;

    clear();
    _dict = dict;
    _dict.remove("cached");

    if (!_dict.value("time").toULongLong()) {
        _dict.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    }

    auto fields = dict.value("fields").value<QVariantList>();

    xbus::node::usr::cmd_t cmd_cnt = 0;
    for (auto const &i : fields) {
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

    // update descr and help from meta DB cache
    storage->loadNodeMeta();

    backup();

    if (_protocol) {
        if (!_dict.value("cached").toBool())
            storage->saveNodeDict();
        if (!_nodes->vehicle->isLocal() || _nodes->vehicle->active())
            _protocol->requestConf();
    }
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

bool NodeItem::loadConfigValue(const QString &name, const QString &value)
{
    for (auto f : m_fields) {
        if (f->fpath() != name)
            continue;
        f->fromVariant(value);
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
            //qWarning() << "missing data for:" << fpath;
            continue;
        }
        fields.append(fpath);
        f->fromVariant(values.value(fpath));
    }

    if (valid()) {
        backup();
        return;
    }

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

    if (_protocol && !_ident.value("reconf").toBool())
        storage->saveNodeConfig();
}
void NodeItem::confUpdated(QVariantMap values)
{
    // updated from another GCS instance
    if (!valid())
        return;
    for (auto name : values.keys()) {
        auto f = field(name);
        if (!f)
            continue;
        f->fromVariant(values.take(name));
        f->backup();
        apxMsg() << tr("Updated").append(':') << name.append(':') << f->text();
    }
    if (values.isEmpty())
        return;

    apxMsgW() << tr("Fields not found").append(':') << values.keys();
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
