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
#include "NodeField.h"
#include "NodeItem.h"
#include "NodeViewActions.h"
#include "Nodes.h"

#include <App/AppLog.h>
#include <App/AppRoot.h>
#include <Pawn/PawnCompiler.h>
#include <Protocols/ProtocolNode.h>
#include <Vehicles/Vehicles.h>

NodeField::NodeField(Fact *parent,
                     NodeItem *node,
                     xbus::node::conf::fid_t fid,
                     const ProtocolNode::dict_field_s &field,
                     NodeField *parentField)
    : Fact(parent, field.name, field.title)
    , _node(node)
    , _parentField(parentField)
    , m_fid(fid)
    , _type(field.type)
    , _fpath(field.path)
{
    new NodeViewActions(this, node->nodes());

    if (field.array && !parentField) {
        QStringList st = field.units.split(',');
        if (st.size() != field.array)
            st.clear();
        for (int i = 0; i < field.array; ++i) {
            ProtocolNode::dict_field_s field_item = field;
            QString s;
            if (st.isEmpty())
                s = QString::number(i + 1);
            else {
                s = st.at(i);
                field_item.units.clear();
            }
            field_item.name.append(QString("_%1").arg(s.toLower()));
            field_item.title = s;
            NodeField *f = new NodeField(this, node, fid, field_item, this);
            connect(f, &Fact::valueChanged, this, &NodeField::updateStatus, Qt::QueuedConnection);
        }
        setTreeType(Group);
        setOption(ModifiedGroup);
        updateStatus();
        return;
    }

    setOption(ModifiedTrack);
    setUnits(field.units);

    switch (field.type) {
    default:
        qDebug() << "Unknown node field data type" << field.type << name() << descr();
        break;
    case xbus::node::conf::real:
        setDataType(Float);
        setPrecision(6);
        break;
    case xbus::node::conf::byte:
        setMax(255);
        setMin(0);
        setDataType(Int);
        break;
    case xbus::node::conf::word:
        setMax(65535);
        setMin(0);
        setDataType(Int);
        break;
    case xbus::node::conf::dword:
        setMin(0);
        setDataType(Int);
        break;
    case xbus::node::conf::bind:
        setDataType(Int);
        setUnits("mandala");
        break;
    case xbus::node::conf::option:
        setDataType(Enum);
        if (field.units.isEmpty()) {
            setEnumStrings(QStringList() << "off"
                                         << "on");
        } else {
            setEnumStrings(field.units.split(','));
        }
        setUnits(QString());
        break;
    case xbus::node::conf::text:
    case xbus::node::conf::string:
        setDataType(Text);
        break;
    case xbus::node::conf::script:
        setDataType(Text);
        setUnits("script");
        if (scriptCompiler)
            break;
        scriptCompiler = new PawnCompiler(this);
        connect(
            this,
            &Fact::valueChanged,
            scriptCompiler,
            [this]() { scriptCompiler->compile(value().toString()); },
            Qt::QueuedConnection);
        connect(
            scriptCompiler,
            &PawnCompiler::compiled,
            this,
            [this]() {
                if (!_node->protocol()->valid())
                    return;
                //apxMsgW() << scriptCompiler->error() << _node->protocol()->valid();
                if (scriptCompiler->error()
                    || _node->protocol()->scriptCode() != scriptCompiler->outData()) {
                    setModified(true);
                }
                updateText();
            },
            Qt::QueuedConnection);
        updateText();
        break;
    }
}

void NodeField::updateStatus()
{
    //arrays only

    if (size() == 3 && _type == xbus::node::conf::real) {
        QStringList st;
        for (int i = 0; i < size(); ++i) {
            st.append(child(i)->text());
        }
        Fact::setValue(QString("(%1)").arg(st.join(',')));
    }

    int acnt = 0;
    for (auto i : facts()) {
        if (i->isZero())
            continue;
        QString s = i->text();
        if (s.isEmpty())
            continue;
        if (s == "0")
            continue;
        acnt++;
    }
    if (acnt > 0)
        Fact::setValue(QString("[%1/%2]").arg(acnt).arg(size()));
    else
        Fact::setValue(QString("[%1]").arg(size()));
}

bool NodeField::setValue(const QVariant &v)
{
    bool isList = _check_type(v, QMetaType::QVariantList);
    if (size() > 0) {
        //expanded field - i.e. array
        if (isList) {
            const QVariantList &values = v.value<QVariantList>();
            if (values.size() > size())
                return false;
            bool rv = false;
            for (int i = 0; i < values.size(); ++i) {
                Fact *f = child(i);
                rv |= f->setValue(values.at(i));
            }
            return rv;
        }
        qWarning() << path() << v;
        return false;
    }
    return Fact::setValue(v);
}
QVariant NodeField::uploadableValue(void) const
{
    if (size() > 0) {
        //expanded field
        QVariantList list;
        for (auto i : facts()) {
            list.append(static_cast<NodeField *>(i)->uploadableValue());
        }
        return list;
    }
    if (_type == xbus::node::conf::real)
        return QString::number(value().toFloat());

    if (_type == xbus::node::conf::script) {
        QByteArray data;
        if (!scriptCompiler->outData().isEmpty())
            data = scriptFileData();
        return QVariant::fromValue(data);
    }

    return value();
}

QString NodeField::toolTip() const
{
    if (!_help.isEmpty()) {
        return Fact::toolTip() + "\n\n" + _help;
    }
    return Fact::toolTip();
}
QString NodeField::toText(const QVariant &v) const
{
    if (_type == xbus::node::conf::script && scriptCompiler) {
        if (scriptCompiler->error())
            return tr("error");
        if (v.toString().trimmed().isEmpty())
            return tr("empty");
        QString title = _node->protocol()->scriptTitle();
        QString s = AppRoot::capacityToString(scriptFileData().size(), 2);
        if (!title.isEmpty())
            s = QString("%1 (%2)").arg(title).arg(s);
        return s;
    }
    return Fact::toText(v);
}

QString NodeField::toString() const
{
    if (_type == xbus::node::conf::script) {
        QString src = value().toString().trimmed();
        if (src.isEmpty())
            return src;
        return qCompress(src.toUtf8(), 9).toHex().toUpper();
    }
    return text();
}
void NodeField::fromString(const QString &s)
{
    if (_type == xbus::node::conf::script) {
        QString src = qUncompress(QByteArray::fromHex(s.toUtf8()));
        if (src.isEmpty())
            src = s;
        Fact::setValue(src);
        return;
    }
    Fact::setValue(s);
}

xbus::node::conf::fid_t NodeField::fid() const
{
    return m_fid;
}

QByteArray NodeField::scriptFileData() const
{
    return _node->protocol()->scriptFileData(value().toString().trimmed(),
                                             scriptCompiler->outData());
}
