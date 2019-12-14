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
#include "Nodes.h"
#include <Pawn/PawnCompiler.h>
#include <Protocols/ProtocolServiceNode.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
NodeField::NodeField(NodeItem *node,
                     quint16 id,
                     int dtype,
                     const QString &name,
                     const QString &title,
                     const QString &descr,
                     const QString &units,
                     const QStringList &opts,
                     const QStringList &groups,
                     NodeField *parentField)
    : NodesBase(parentField, name, title, descr, NoFlags)
    , id(id)
    , dtype(dtype)
    , groups(groups)
    , node(node)
    , pawncc(nullptr)
    , parentField(parentField)
{
    setUnits(units);
    setEnumStrings(opts);

    switch (dtype) {
    default:
        qDebug() << "Unknown node field data type" << dtype << name << descr;
        break;
    case DictNode::Float:
        setDataType(Float);
        if (!name.startsWith("mekf_"))
            setPrecision(4);
        break;
    case DictNode::Byte:
        setMax(255);
        //fallthru
    case DictNode::UInt:
        setMin(0);
        setDataType(Int);
        break;
    case DictNode::Option:
        setDataType(Enum);
        break;
    case DictNode::String:
    case DictNode::StringL:
        setDataType(Text);
        break;
    case DictNode::MandalaID:
        setDataType(Mandala);
        setEnumStrings(QStringList());
        break;
    case DictNode::Script:
        setDataType(Script);
        if (pawncc)
            break;
        pawncc = new PawnCompiler(this);
        connect(this, &Fact::valueChanged, pawncc, &PawnCompiler::compile);
        connect(pawncc, &PawnCompiler::compiled, this, &NodeField::updateStatus);
        connect(
            pawncc,
            &PawnCompiler::compiled,
            this,
            [this]() {
                if (dataValid() && (!scriptCodeSave.isEmpty())
                    && scriptCodeSave != pawncc->outData()) {
                    setModified(true);
                }
            },
            Qt::QueuedConnection);
        break;
        //field with subfields
    case DictNode::Hash:
    case DictNode::Vector:
    case DictNode::Array:
        setTreeType(Group);
        break;
    }

    if (!parentField) {
        connect(this, &NodeField::dataValidChanged, this, &NodeField::validateData);

        connect(this, &NodesBase::dataValidChanged, this, [=]() { setEnabled(dataValid()); });
        setEnabled(dataValid());

        addActions();
        node->allFields.append(this);

        //check if comment field and bind to node value
        if (name == "comment" || name == "node_label" || (name == "name" && id == 0)) {
            connect(this, &NodeField::textChanged, node, [=]() {
                node->setStatus(text().trimmed());
            });
        }
    } else {
        //expanded sub field
        connect(this, &NodeField::valueChanged, parentField, &NodeField::updateStatus);
        parentField->updateStatus();

        connect(this, &NodesBase::dataValidChanged, this, [=]() { setEnabled(dataValid()); });
        setEnabled(dataValid());
    }
}
//=============================================================================
void NodeField::updateStatus()
{
    if (dtype == DictNode::Vector) {
        QStringList st;
        for (int i = 0; i < size(); ++i) {
            NodeField *f = child<NodeField>(i);
            st.append(f->text());
        }
        setStatus(QString("(%1)").arg(st.join(',')));
    } else if (dtype == DictNode::Array) {
        int acnt = 0;
        for (int i = 0; i < size(); ++i) {
            NodeField *f = child<NodeField>(i);
            if (f->isZero())
                continue;
            QString s = f->text();
            if (s.isEmpty())
                continue;
            if (s == "0")
                continue;
            acnt++;
        }
        if (acnt > 0)
            setStatus(QString("[%1/%2]").arg(acnt).arg(size()));
        else
            setStatus(QString("[%1]").arg(size()));
    } else if (dtype == DictNode::Script) {
        if (pawncc->error())
            setStatus("<" + tr("error") + ">");
        else if (value().toString().isEmpty())
            setStatus("<" + tr("empty") + ">");
        else
            setStatus(QString("~%1 Kb").arg((text().size() + pawncc->outData().size()) / 1024.0,
                                            0,
                                            'f',
                                            1));
    }
}
//=============================================================================
void NodeField::setModified(const bool &v, const bool &recursive)
{
    if (m_modified == v)
        return;
    FactData::setModified(v, recursive);
    const Vehicle *vehicle = findParent<Vehicle *>();
    if (v) {
        //qDebug()<<"mod"<<path();
        //set all parents to modified=true
        for (Fact *i = parentFact(); i != vehicle; i = i->parentFact()) {
            if (i)
                i->setModified(v);
            else
                break;
        }
        return;
    }
    //refresh modified status of all parent items
    for (Fact *i = parentFact(); i && i != vehicle; i = i->parentFact()) {
        for (int j = 0; j < i->size(); ++j) {
            Fact *f = i->child(j);
            if (f) {
                if (f->modified())
                    return;
            } else
                break;
        }
        i->setModified(v);
    }
}
//=============================================================================
bool NodeField::setValue(const QVariant &v)
{
    if (size() > 0) {
        //expanded field
        const QVariantList &values = v.value<QVariantList>();
        if (values.size() != size())
            return false;
        bool rv = false;
        for (int i = 0; i < size(); ++i) {
            Fact *f = child(i);
            rv |= f->setValue(values.at(i));
        }
        return rv;
    }
    if (dtype == DictNode::Script && vtype(v, QMetaType::QVariantList)) {
        QVariantList pkg = v.value<QVariantList>();
        QString src = pkg.value(0).toString();
        scriptCodeSave = pkg.value(1).toByteArray();
        //qDebug()<<scriptCodeSave.size();
        return Fact::setValue(src);
    }
    return Fact::setValue(v);
}
QVariant NodeField::uploadableValue(void) const
{
    if (size() > 0) {
        //expanded field
        QVariantList values;
        for (int i = 0; i < size(); ++i) {
            Fact *f = child(i);
            values.append(f->value());
        }
        return values;
    }
    if (dtype == DictNode::Script) {
        QVariantList pkg;
        if (!pawncc->outData().isEmpty())
            pkg << value() << pawncc->outData();
        return pkg;
    }
    return value();
}
//=============================================================================
QString NodeField::toString() const
{
    QString v = text();
    if (dtype == DictNode::Script && (!v.isEmpty())) {
        v = qCompress(v.toUtf8(), 9).toHex().toUpper();
    }
    return v;
}
void NodeField::fromString(const QString &s)
{
    if (dtype == DictNode::Script && (!s.isEmpty())) {
        QByteArray ba = qUncompress(QByteArray::fromHex(s.toUtf8()));
        if (!ba.isEmpty()) {
            setValue(QString(ba));
            return;
        }
    }
    setValue(s);
}
//=============================================================================
//=============================================================================
void NodeField::validateData()
{
    if (!dataValid())
        return;
    backup();
}
//=============================================================================
//=============================================================================
void NodeField::hashData(QCryptographicHash *h) const
{
    Fact::hashData(h);
    h->addData(QString::number(id).toUtf8());
    h->addData(DictNode::dataTypeToString(dtype).toUtf8());
}
//=============================================================================
