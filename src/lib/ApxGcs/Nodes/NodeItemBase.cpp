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
//=============================================================================
NodeItemBase::NodeItemBase(Fact *parent,
                           const QString &name,
                           const QString &title,
                           Fact::Flags flags)
    : NodesBase(parent, name, title, "", flags)
    , progress_s(0)
    , m_dictValid(false)
    , m_upgrading(false)
{
    //parent forward check
    connect(this, &NodeItemBase::dictValidChanged, this, &NodeItemBase::updateDictValid);
    updateDictValid();

    connect(this, &NodeItemBase::upgradingChanged, this, &NodeItemBase::updateUpgrading);

    connect(this, &Fact::progressChanged, this, &NodeItemBase::updateProgress);

    //force update modelViews
    connect(this, &NodeItemBase::dictValidChanged, this, &Fact::enabledChanged);
    connect(this, &NodeItemBase::upgradingChanged, this, &Fact::enabledChanged);
}
QStringList NodeItemBase::sortNames = QStringList() << "shiva"
                                                    << "nav"
                                                    << "cas"
                                                    << "gps"
                                                    << "ifc"
                                                    << "swc"
                                                    << "mhx"
                                                    << "servo"
                                                    << "bldc";
//=============================================================================
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
        break;
    case Qt::BackgroundRole:
        if (dictValid() && dataValid()) {
            return QColor(0x10, 0x20, 0x30);
        }
        if (upgrading())
            return QColor(64, 0, 0);
        if (!dictValid())
            return QVariant();
        break;
    }
    return NodesBase::data(col, role);
}
//=============================================================================
bool NodeItemBase::lessThan(Fact *rightFact) const
{
    //try to sort by sortNames
    QString sleft = title().toLower();
    if (sleft.contains('.'))
        sleft = sleft.remove(0, sleft.indexOf('.') + 1).trimmed();
    QString sright = rightFact->title().toLower();
    if (sright.contains('.'))
        sright = sright.remove(0, sright.indexOf('.') + 1).trimmed();
    if (sortNames.contains(sleft)) {
        if (sortNames.contains(sright)) {
            int ileft = sortNames.indexOf(sleft);
            int iright = sortNames.indexOf(sright);
            if (ileft != iright)
                return ileft < iright;
        } else
            return true;
    } else if (sortNames.contains(sright))
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
//=============================================================================
//=============================================================================
void NodeItemBase::updateDictValid()
{
    if (!dictValid())
        setDataValid(false, false);
    NodeItemBase *p = qobject_cast<NodeItemBase *>(parentFact());
    if (!p)
        return;
    bool ok = true;
    for (int i = 0; i < p->size(); ++i) {
        const NodeItemBase *item = static_cast<NodeItemBase *>(p->child(i));
        if (item->dictValid())
            continue;
        ok = false;
        break;
    }
    p->setDictValid(ok);
    //qDebug()<<ok<<p->name()<<name();
}
//=============================================================================
void NodeItemBase::updateUpgrading()
{
    /*if(upgrading()){
    setProgress(0);
  }else{
    setProgress(-1);
  }*/
    NodeItemBase *p = qobject_cast<NodeItemBase *>(parentFact());
    if (!p)
        return;
    //if(qobject_cast<Nodes*>(p))return;
    //set parent upgrading if any of its child is upgrading
    bool ok = false;
    for (int i = 0; i < p->size(); ++i) {
        const NodeItemBase *item = static_cast<NodeItemBase *>(p->child(i));
        if (!item->upgrading())
            continue;
        ok = true;
        break;
    }
    p->setUpgrading(ok);
}
//=============================================================================
void NodeItemBase::updateProgress()
{
    NodeItemBase *p = qobject_cast<NodeItemBase *>(parentFact());
    if (!p)
        return;
    int ncnt = 0, v = 0;
    for (int i = 0; i < p->size(); ++i) {
        const NodeItemBase *item = static_cast<NodeItemBase *>(p->child(i));
        int np = item->progress();
        if (np < 0)
            continue;
        ncnt++;
        v += np;
    }
    if (ncnt > 0) {
        if (p->progress_s < ncnt) {
            p->progress_s = ncnt;
            //qDebug()<<"progressCnt"<<p->progress_s<<ncnt<<p->name()<<name();
        } else if (ncnt < p->progress_s) {
            v += (p->progress_s - ncnt) * 100;
            ncnt = p->progress_s;
        }
        p->setProgress(v / ncnt);
    } else {
        p->progress_s = 0;
        p->setProgress(-1);
        //qDebug()<<"progressCnt"<<p->progress_s;
    }
}
//=============================================================================
//=============================================================================
//=============================================================================
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
//=============================================================================
//=============================================================================
