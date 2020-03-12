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
#include "NodeItemBase.h"
#include "NodeItem.h"
#include "Nodes.h"

QStringList NodeItemBase::g_sortNames = {"shiva", "nav", "ifc", "cas", "gps", "dlink", "servo"};

NodeItemBase::NodeItemBase(Fact *parent,
                           const QString &name,
                           const QString &title,
                           Fact::Flags flags)
    : NodesBase(parent, name, title, "", flags)
{
    //parent forward check
    connect(this, &NodeItemBase::dictValidChanged, this, &NodeItemBase::updateDictValid);
    updateDictValid();
    connect(this, &NodeItemBase::dataValidChanged, this, &NodeItemBase::updateDataValid);
    updateDataValid();

    //force update modelViews
    connect(this, &NodeItemBase::dictValidChanged, this, &Fact::enabledChanged);
    connect(this, &NodeItemBase::dataValidChanged, this, &Fact::enabledChanged);

    connect(this, &NodeItemBase::upgradingChanged, this, &NodeItemBase::updateUpgrading);

    //force update modelViews
    connect(this, &NodeItemBase::dictValidChanged, this, &Fact::enabledChanged);
    connect(this, &NodeItemBase::upgradingChanged, this, &Fact::enabledChanged);
}

QVariant NodeItemBase::data(int col, int role) const
{
    switch (role) {
    case Qt::ForegroundRole:
        if (dictValid() && dataValid()) {
            if (col == FACT_MODEL_COLUMN_DESCR)
                return QColor(Qt::darkGray);
            if (col == FACT_MODEL_COLUMN_VALUE)
                return QColor(Qt::yellow).lighter(180);
        }
        if (upgrading())
            return QColor(255, 200, 200);
        if (!dictValid())
            return QColor(255, 200, 200);
        if (!dataValid())
            return col == FACT_MODEL_COLUMN_NAME ? QColor(255, 255, 200) : QColor(Qt::darkGray);
        break;
    case Qt::BackgroundRole:
        if (dictValid() && dataValid()) {
            return QColor(0x10, 0x20, 0x30);
        }
        if (upgrading())
            return QColor(64, 0, 0);
        if (!dictValid())
            return QVariant();
        if (!dataValid())
            return QVariant();
        break;
    }
    return NodesBase::data(col, role);
}

bool NodeItemBase::lessThan(Fact *rightFact) const
{
    //try to sort by sortNames
    QString sleft = title().toLower();
    if (sleft.contains('.'))
        sleft = sleft.remove(0, sleft.indexOf('.') + 1).trimmed();
    QString sright = rightFact->title().toLower();
    if (sright.contains('.'))
        sright = sright.remove(0, sright.indexOf('.') + 1).trimmed();
    if (g_sortNames.contains(sleft)) {
        if (g_sortNames.contains(sright)) {
            int ileft = g_sortNames.indexOf(sleft);
            int iright = g_sortNames.indexOf(sright);
            if (ileft != iright)
                return ileft < iright;
        } else
            return true;
    } else if (g_sortNames.contains(sright))
        return false;

    //compare names
    int ncmp = QString::localeAwareCompare(title().toLower(), rightFact->title().toLower());
    if (ncmp != 0)
        return ncmp < 0;
    //try to sort by comment same names
    ncmp = QString::localeAwareCompare(text().toLower(), rightFact->text().toLower());
    if (ncmp == 0) {
        //try to sort by sn same names
        const NodeItem *lnode = qobject_cast<const NodeItem *>(this);
        const NodeItem *rnode = qobject_cast<const NodeItem *>(rightFact);
        if (lnode && rnode)
            ncmp = QString::localeAwareCompare(lnode->sn(), rnode->sn());
    }
    if (ncmp == 0)
        return Fact::lessThan(rightFact);
    return ncmp < 0;
}

QList<const NodeItemBase *> NodeItemBase::groupNodesList() const
{
    QList<const NodeItemBase *> list;
    NodeItemBase *p = qobject_cast<NodeItemBase *>(parentFact());
    if (!p)
        return list;
    for (int i = 0; i < p->size(); ++i) {
        const NodeItemBase *item = qobject_cast<NodeItemBase *>(p->child(i));
        if (item)
            list.append(item);
    }
    return list;
}

void NodeItemBase::updateDictValid()
{
    if (!dictValid())
        setDataValid(false);
    //update node group
    QList<const NodeItemBase *> list = groupNodesList();
    if (list.isEmpty())
        return;
    bool ok = true;
    for (auto i : list) {
        if (i->dictValid())
            continue;
        ok = false;
        break;
    }
    qobject_cast<NodeItemBase *>(parentFact())->setDictValid(ok);
    //qDebug()<<ok<<p->name()<<name();
}
void NodeItemBase::updateDataValid()
{
    QList<const NodeItemBase *> list = groupNodesList();
    if (list.isEmpty())
        return;
    //update node group
    bool ok = true;
    for (auto i : list) {
        if (i->dataValid())
            continue;
        ok = false;
        break;
    }
    qobject_cast<NodeItemBase *>(parentFact())->setDataValid(ok);
}

void NodeItemBase::updateUpgrading()
{
    QList<const NodeItemBase *> list = groupNodesList();
    if (list.isEmpty())
        return;
    //update node group
    bool ok = false;
    for (auto i : list) {
        if (!i->upgrading())
            continue;
        ok = true;
        break;
    }
    qobject_cast<NodeItemBase *>(parentFact())->setUpgrading(ok);
}

bool NodeItemBase::dictValid() const
{
    return m_dictValid;
}
void NodeItemBase::setDictValid(const bool &v)
{
    if (m_dictValid == v)
        return;
    m_dictValid = v;
    emit dictValidChanged();
}
bool NodeItemBase::dataValid() const
{
    return m_dataValid;
}
void NodeItemBase::setDataValid(const bool &v)
{
    if (m_dataValid == v)
        return;
    m_dataValid = v;
    emit dataValidChanged();
}
bool NodeItemBase::upgrading() const
{
    return m_upgrading;
}
void NodeItemBase::setUpgrading(const bool &v)
{
    if (m_upgrading == v)
        return;
    m_upgrading = v;
    emit upgradingChanged();
}
