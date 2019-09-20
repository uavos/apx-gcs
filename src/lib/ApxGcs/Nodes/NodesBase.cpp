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
#include "NodesBase.h"
#include "Nodes.h"
//=============================================================================
NodesBase::NodesBase(
    Fact *parent, const QString &name, const QString &title, const QString &descr, Flags flags)
    : Fact(parent, name, title, descr, flags)
    , m_dataValid(false)
{
    //parent forward check
    connect(this, &NodesBase::dataValidChanged, this, &NodesBase::updateDataValid);
    updateDataValid();

    //force update modelViews
    connect(this, &NodesBase::dataValidChanged, this, &Fact::enabledChanged);

    connect(this, &Fact::parentFactChanged, this, &NodesBase::addActions);
    addActions();
}
//=============================================================================
void NodesBase::addActions()
{
    if (!parentFact())
        return;
    if (!actions().isEmpty())
        return;

    disconnect(this, &Fact::parentFactChanged, this, &NodesBase::addActions);

    f_revert = new Fact(this, "revert", tr("Revert"), tr("Undo changes"), Action, "undo");
    connect(f_revert, &Fact::triggered, this, &Fact::restore);
    connect(this, &Fact::modifiedChanged, f_revert, [this]() { f_revert->setEnabled(modified()); });
    f_revert->setEnabled(modified());

    Nodes *nodes = findParent<Nodes *>();
    if (!nodes)
        return;
    Fact *f = new Fact(this, nodes->f_upload->name(), "", "", Action);
    f->bind(nodes->f_upload);
}
//=============================================================================
QVariant NodesBase::data(int col, int role) const
{
    switch (role) {
    case Qt::ForegroundRole:
        if (!dataValid())
            return col == FACT_MODEL_COLUMN_NAME ? QColor(255, 255, 200) : QColor(Qt::darkGray);
        break;
    case Qt::BackgroundRole:
        if (!dataValid())
            return QVariant();
        break;
    }
    return Fact::data(col, role);
}
//=============================================================================
void NodesBase::updateDataValid()
{
    NodesBase *p = qobject_cast<NodesBase *>(parentFact());
    if (!p)
        return;
    bool ok = true;
    for (int i = 0; i < p->size(); ++i) {
        const NodesBase *item = static_cast<NodesBase *>(p->child(i));
        if (item->dataValid())
            continue;
        ok = false;
        break;
    }
    p->setDataValid(ok, false);
}
//=============================================================================
//=============================================================================
bool NodesBase::dataValid() const
{
    return m_dataValid;
}
void NodesBase::setDataValid(const bool &v, bool recursive)
{
    if (recursive) {
        for (int i = 0; i < size(); ++i) {
            static_cast<NodesBase *>(child(i))->setDataValid(v);
        }
    }
    if (m_dataValid == v)
        return;
    m_dataValid = v;
    emit dataValidChanged();
}
//=============================================================================
//=============================================================================
