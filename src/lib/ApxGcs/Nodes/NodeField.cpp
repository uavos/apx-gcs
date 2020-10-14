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
        _script = new ScriptCompiler(this);
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
        setValue(QString("(%1)").arg(st.join(',')));
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
        setValue(QString("[%1/%2]").arg(acnt).arg(size()));
    else
        setValue(QString("[%1]").arg(size()));
}

QVariant NodeField::confValue(void) const
{
    if (size() > 0) {
        //expanded field
        QVariantList list;
        for (auto i : facts()) {
            list.append(static_cast<NodeField *>(i)->confValue());
        }
        return list;
    }
    if (_type == xbus::node::conf::real)
        return QString::number(value().toFloat());

    return value();
}
void NodeField::setConfValue(const QVariant &v)
{
    bool isList = _check_type(v, QMetaType::QVariantList);
    if (size() > 0) {
        //expanded field - i.e. array
        if (isList) {
            const QVariantList &values = v.value<QVariantList>();
            if (values.size() > size())
                return;
            for (int i = 0; i < values.size(); ++i) {
                child(i)->setValue(values.at(i));
            }
            return;
        }
        qWarning() << path() << v;
        return;
    }
    setValue(v);
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
    if (_type == xbus::node::conf::script) {
        QStringList st = v.toString().split(',', Qt::KeepEmptyParts);
        QString title;
        QString src;
        QByteArray code;
        size_t size = 0;
        if (st.size() == 3) {
            title = st.at(0);
            QByteArray ba = QByteArray::fromHex(st.at(1).toLocal8Bit());
            size += ba.size();
            src = qUncompress(ba);
            code = qUncompress(QByteArray::fromHex(st.at(2).toLocal8Bit()));
            size += code.size();
        }
        if (src.isEmpty() && code.isEmpty())
            return tr("empty");
        if (code.isEmpty())
            return tr("error");
        QString s = AppRoot::capacityToString(size, 2);
        if (!title.isEmpty())
            s = QString("%1 (%2)").arg(title).arg(s);
        return s;
    }
    return Fact::toText(v);
}

xbus::node::conf::fid_t NodeField::fid() const
{
    return m_fid;
}
