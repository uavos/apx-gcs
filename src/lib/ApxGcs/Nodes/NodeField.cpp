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
#include "NodeField.h"
#include "NodeItem.h"
#include "NodeViewActions.h"
#include "Nodes.h"

#include <App/AppLog.h>
#include <App/AppRoot.h>
#include <Protocols/ProtocolNode.h>
#include <Vehicles/Vehicles.h>

NodeField::NodeField(
    Fact *parent, NodeItem *node, QJsonObject json, size_t id, NodeField *arrayParent)
    : Fact(parent)
    , _node(node)
    , _type(json.value("type").toString())
    , _id(id)
    , _fpath(json.value("name").toString())
{
    setName(_fpath.split('.').last());
    setTitle(json.value("title").toString());

    QString funits = json.value("units").toString();
    _array = json.value("array").toInt();

    new NodeViewActions(this, node->nodes());

    if (_array && !arrayParent) {
        // create array sub fields
        QStringList st = funits.split(',');
        if (st.size() != _array)
            st.clear();
        for (auto i = 0; i < _array; ++i) {
            QJsonObject item = json;
            QString s;
            if (st.isEmpty())
                s = QString::number(i + 1);
            else {
                s = st.at(i);
                item.remove("units");
            }
            item.insert("name", QString("%1_%2").arg(_fpath).arg(s.toLower()));
            item.insert("title", s);
            item.insert("array", i);
            NodeField *f = new NodeField(this, node, item, id, this);
            connect(f, &Fact::valueChanged, this, &NodeField::updateStatus, Qt::QueuedConnection);
        }
        setTreeType(Group);
        setOption(ModifiedGroup);
        updateStatus();
        return;
    }

    setOption(ModifiedTrack);
    setUnits(funits);

    if (_type == "real") {
        setDataType(Float);
        setPrecision(6);
    } else if (_type == "byte") {
        setMax(255);
        setMin(0);
        setDataType(Int);
    } else if (_type == "word") {
        setMax(65535);
        setMin(0);
        setDataType(Int);
    } else if (_type == "dword") {
        setMin(0);
        setDataType(Int);
    } else if (_type == "bind") {
        setDataType(Int);
        setUnits("mandala");
    } else if (_type == "option") {
        setDataType(Enum);
        if (funits.isEmpty()) {
            setEnumStrings(QStringList() << "off"
                                         << "on");
        } else {
            setEnumStrings(funits.split(','));
        }
        setUnits(QString());
    } else if (_type == "text" || _type == "string") {
        setDataType(Text);
    } else if (_type == "script") {
        setDataType(Text);
        setUnits("script");
        _script = new NodeScript(this);
    } else {
        qDebug() << "Unknown node field data type" << _type << name() << descr();
    }
}

void NodeField::updateStatus()
{
    //arrays only

    if (size() == 3 && _type == "real") {
        QStringList st;
        for (int i = 0; i < size(); ++i) {
            st.append(child(i)->valueText());
        }
        setValue(QString("(%1)").arg(st.join(',')));
    }

    int acnt = 0;
    for (auto i : facts()) {
        if (i->isZero())
            continue;
        auto s = i->valueText();
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
    if (_type == "real")
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
    if (_type == "script")
        return QString();
    return Fact::toText(v);
}
