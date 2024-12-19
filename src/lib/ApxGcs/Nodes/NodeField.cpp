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
#include <ApxMisc/JsonHelpers.h>
#include <Fleet/Fleet.h>

NodeField::NodeField(
    Fact *parent, NodeItem *node, const QJsonObject &m, size_t id, NodeField *arrayParent)
    : Fact(parent)
    , _node(node)
    , _type(m.value("type").toString())
    , _id(id)
    , _fpath(m.value("name").toString())
{
    setName(_fpath.split('.').last());
    setTitle(m.value("title").toString());

    auto funits = m.value("units").toString();
    _array = m.value("array").toVariant().toInt();

    new NodeViewActions(this, node->nodes());

    if (_array && !arrayParent) {
        // create array sub fields
        auto st = funits.split(',');
        if (st.size() != _array || _type == "option")
            st.clear();
        for (auto i = 0; i < _array; ++i) {
            auto item = m;
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
            auto f = new NodeField(this, node, item, id, this);
            connect(f,
                    &Fact::valueChanged,
                    this,
                    &NodeField::updateArrayStatus,
                    Qt::QueuedConnection);
        }
        setTreeType(Group);
        setOption(ModifiedGroup);
        updateArrayStatus();
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
        setEnumStrings(funits.split(','));
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

void NodeField::updateArrayStatus()
{
    // called for arrays only

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

QString NodeField::toText(const QVariant &v) const
{
    if (_type == "script")
        return QString();
    return Fact::toText(v);
}

QJsonValue NodeField::toJson()
{
    if (isZero())
        return {};

    QJsonValue jsv;

    if (size() > 0) {
        //expanded field (array)
        QJsonArray jsa;
        for (auto i : facts()) {
            jsa.append(static_cast<NodeField *>(i)->toJson());
        }
        // remove tail null values
        while (!jsa.isEmpty() && jsa.last().isNull())
            jsa.removeLast();
        jsv = jsa.isEmpty() ? QJsonValue() : jsa;
    } else if (_type == "script") {
        jsv = QJsonValue::fromVariant(value());
    } else if (_type == "real") {
        // use floats as-is (not text)
        jsv = QJsonValue::fromVariant(value());
    } else {
        jsv = valueText();
    }

    return json::fix_number(jsv);
}
void NodeField::fromJson(const QJsonValue &jsv)
{
    if (size() > 0) {
        // expanded field
        // json::save("nodes-field-" + title(), jsv);

        if (!jsv.isArray() && !jsv.isNull()) {
            qWarning() << "Array expected" << path();
            return;
        }
        const auto jsa = jsv.toArray();
        int i = 0;
        while (i < jsa.size() && i < size()) {
            child(i)->fromJson(jsa.at(i));
            i++;
        }
        // set all missing array elements to zero
        while (i < size()) {
            // qDebug() << "zero" << fpath() << i;
            child(i)->setValue({});
            i++;
        }
        return;
    }

    if (_type == "real") {
        setValue(QString::number(jsv.toVariant().toDouble()));
        return;
    }

    setValue(jsv.toVariant());
}
