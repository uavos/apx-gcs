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
#include "FactTreeModel.h"
#include <Fact/Fact.h>
#include <QDomDocument>
//=============================================================================
FactTreeModel::FactTreeModel(Fact *root)
    : QAbstractItemModel(root)
    , root(root)
{
    updateTimer.setSingleShot(true);
    updateTimer.setInterval(500);
    connect(&updateTimer, &QTimer::timeout, this, &FactTreeModel::updateTimerTimeout);
}
//=============================================================================
//=============================================================================
QHash<int, QByteArray> FactTreeModel::roleNames() const
{
    return root->model()->roleNames();
}
//=============================================================================
QVariant FactTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    Fact *f = fact(index);
    if (!f)
        return QVariant();
    return f->data(index.column(), role);
}
//=============================================================================
bool FactTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ((!index.isValid()) || (role != Qt::EditRole)
        || index.column() != Fact::FACT_MODEL_COLUMN_VALUE)
        return false;
    Fact *f = fact(index);
    if (!f)
        return false;
    if (data(index, role) == value)
        return true;
    bool rv = f->setValue(value);
    updateTimerTimeout();
    //if(rv)emit dataChanged(index,index);//layoutChanged();
    return rv;
}
//=============================================================================
//=============================================================================
QModelIndex FactTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    Fact *parentFact;
    if (!parent.isValid())
        parentFact = root;
    else
        parentFact = fact(parent);
    if (!parentFact)
        return QModelIndex();
    Fact *childFact = parentFact->child(row);
    if (!childFact)
        return QModelIndex();
    checkConnections(childFact);
    return factIndex(childFact, column);
}
//=============================================================================
QModelIndex FactTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();
    Fact *i = fact(index);
    if (!i)
        return QModelIndex();
    if (!i->parentFact())
        return QModelIndex();
    Fact *p = i->parentFact();
    if (!p || p == root)
        return QModelIndex();
    checkConnections(p);
    return factIndex(p);
}
//=============================================================================
int FactTreeModel::rowCount(const QModelIndex &parent) const
{
    Fact *parentFact;
    if (parent.column() > 0)
        return 0;
    if (!parent.isValid())
        parentFact = root;
    else
        parentFact = fact(parent);
    return parentFact->size();
}
//=============================================================================
int FactTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return Fact::FACT_MODEL_COLUMN_CNT;
}
//=============================================================================
QVariant FactTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)
    if (role == Qt::DisplayRole) {
        switch (section) {
        case Fact::FACT_MODEL_COLUMN_NAME:
            return tr("Name");
        case Fact::FACT_MODEL_COLUMN_VALUE:
            return tr("Value");
        case Fact::FACT_MODEL_COLUMN_DESCR:
            return tr("Description");
        }
    }
    return QVariant();
}
//=============================================================================
Qt::ItemFlags FactTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags fx = Qt::NoItemFlags;
    Fact *f = fact(index);
    if (!f)
        return fx;
    if (f->enabled())
        fx |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return fx;
    if (index.column() != Fact::FACT_MODEL_COLUMN_VALUE)
        return fx;
    if (f->treeType() == Fact::Group && f->size() > 1 && f->child(0)->treeType() == Fact::Group
        && f->status().startsWith('[') && f->status().endsWith(']'))
        return fx | Qt::ItemIsEditable; //array editor

    if (f->treeType())
        return fx;
    if (f->dataType() && f->dataType() != Fact::Const) {
        fx |= Qt::ItemIsEditable;
    }
    return fx;
}
//=============================================================================
Fact *FactTreeModel::fact(const QModelIndex &index) const
{
    return qobject_cast<Fact *>(static_cast<QObject *>(index.internalPointer()));
}
QModelIndex FactTreeModel::factIndex(FactBase *item, int column) const
{
    if (item == root)
        return QModelIndex();
    return createIndex(item->num(), column, item);
}
//=============================================================================
void FactTreeModel::checkConnections(Fact *fact) const
{
    if (!conFactLayout.contains(fact)) {
        const_cast<QList<Fact *> *>(&conFactLayout)->append(fact);
        connect(fact, &Fact::destroyed, this, &FactTreeModel::itemDestroyed);

        connect(fact, &Fact::itemToBeInserted, this, &FactTreeModel::itemToBeInserted);
        connect(fact, &Fact::itemInserted, this, &FactTreeModel::itemInserted);
        connect(fact, &Fact::itemToBeRemoved, this, &FactTreeModel::itemToBeRemoved);
        connect(fact, &Fact::itemRemoved, this, &FactTreeModel::itemRemoved);
        connect(fact, &Fact::itemToBeMoved, this, &FactTreeModel::itemToBeMoved);
        connect(fact, &Fact::itemMoved, this, &FactTreeModel::itemMoved);

        connect(fact, &Fact::textChanged, this, &FactTreeModel::textChanged);
        connect(fact, &Fact::statusChanged, this, &FactTreeModel::textChanged);
        connect(fact, &Fact::titleChanged, this, &FactTreeModel::titleChanged);
        connect(fact, &Fact::descrChanged, this, &FactTreeModel::descrChanged);
        connect(fact, &Fact::enabledChanged, this, &FactTreeModel::enabledChanged);
        connect(fact, &Fact::activeChanged, this, &FactTreeModel::activeChanged);
        connect(fact, &Fact::modifiedChanged, this, &FactTreeModel::modifiedChanged);
        connect(fact, &Fact::progressChanged, this, &FactTreeModel::progressChanged);
        connect(fact, &Fact::visibleChanged, this, &FactTreeModel::visibleChanged);
    }
}
//=============================================================================
void FactTreeModel::recursiveDisconnect(Fact *fact)
{
    for (int i = 0; i < fact->size(); ++i) {
        recursiveDisconnect(fact->child(i));
    }
    if (!conFactLayout.contains(fact))
        return;
    disconnect(fact, 0, this, 0);

    /*disconnect(fact,&Fact::itemToBeInserted,this,&FactTreeModel::itemToBeInserted);
  disconnect(fact,&Fact::itemInserted,this,&FactTreeModel::itemInserted);
  disconnect(fact,&Fact::itemToBeRemoved,this,&FactTreeModel::itemToBeRemoved);
  disconnect(fact,&Fact::itemRemoved,this,&FactTreeModel::itemRemoved);
  disconnect(fact,&Fact::textChanged, this, &FactTreeModel::textChanged);
  disconnect(fact,&Fact::statusChanged, this, &FactTreeModel::textChanged);
  disconnect(fact,&Fact::titleChanged, this, &FactTreeModel::titleChanged);
  disconnect(fact,&Fact::descrChanged, this, &FactTreeModel::descrChanged);
  disconnect(fact,&Fact::enabledChanged, this, &FactTreeModel::enabledChanged);
  disconnect(fact,&Fact::activeChanged, this, &FactTreeModel::activeChanged);
  disconnect(fact,&Fact::progressChanged, this, &FactTreeModel::progressChanged);*/

    //qDebug()<<"dis"<<fact->path();
    //resetInternalData();
    conFactLayout.removeOne(fact);
}
//=============================================================================
void FactTreeModel::itemToBeInserted(int row, FactBase *item)
{
    Fact *fact = qobject_cast<Fact *>(item->parentFact());
    const QModelIndex &index = factIndex(fact);
    beginInsertRows(index, row, row);
}
void FactTreeModel::itemInserted(FactBase *)
{
    endInsertRows();
}
void FactTreeModel::itemToBeRemoved(int row, FactBase *item)
{
    Fact *fact = qobject_cast<Fact *>(item->parentFact());
    //updateList.removeAll(fact);
    const QModelIndex &index = factIndex(fact);
    beginRemoveRows(index, row, row);
}
void FactTreeModel::itemRemoved(FactBase *)
{
    endRemoveRows();
}
void FactTreeModel::itemToBeMoved(int row, int dest, FactBase *item)
{
    Fact *fact = qobject_cast<Fact *>(item->parentFact());
    const QModelIndex &index = factIndex(fact);
    beginMoveRows(index, row, row, index, dest);
}
void FactTreeModel::itemMoved(FactBase *)
{
    endMoveRows();
}
//=============================================================================
void FactTreeModel::textChanged()
{
    if (!sender())
        return;
    Fact *fact = qobject_cast<Fact *>(sender())->parentFact();
    //qDebug()<<fact->path();
    if (!updateList.contains(fact))
        updateList.append(fact);
    if (!updateHash.values(fact).contains(Fact::FACT_MODEL_COLUMN_VALUE))
        updateHash.insertMulti(fact, Fact::FACT_MODEL_COLUMN_VALUE);
    if (!updateTimer.isActive())
        updateTimer.start();
}
void FactTreeModel::titleChanged()
{
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
}
void FactTreeModel::descrChanged()
{
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_DESCR);
    dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
}
void FactTreeModel::enabledChanged()
{
    QModelIndex index1 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    QModelIndex index2 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_DESCR);
    dataChanged(index1, index2, QVector<int>() << Qt::ForegroundRole << Qt::BackgroundRole);
}
void FactTreeModel::activeChanged()
{
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    dataChanged(index, index, QVector<int>() << Qt::ForegroundRole);
}
void FactTreeModel::modifiedChanged()
{
    QModelIndex index1 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    QModelIndex index2 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_VALUE);
    dataChanged(index1, index2, QVector<int>() << Qt::ForegroundRole);
}
void FactTreeModel::progressChanged()
{
    Fact *fact = qobject_cast<Fact *>(sender());
    if (!expandedFacts.contains(fact->parentFact()))
        return;
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_DESCR);
    dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
}
void FactTreeModel::visibleChanged()
{
    emit layoutAboutToBeChanged();
    emit layoutChanged();
}
//=============================================================================
void FactTreeModel::updateTimerTimeout()
{
    updateTimer.stop();
    foreach (Fact *f, updateList) {
        if (!(f && f->size()))
            continue;
        if (!expandedFacts.contains(f))
            continue;
        foreach (int col, updateHash.values(f)) {
            QModelIndex index1 = createIndex(0, col, f->child(0));
            QModelIndex index2 = createIndex(f->size() - 1, col, f->child(f->size() - 1));
            emit dataChanged(index1, index2, QVector<int>() << Qt::DisplayRole);
            //qDebug()<<f->path();
        }
    }
    //emit dataChanged(QModelIndex(),QModelIndex());
    updateList.clear();
    updateHash.clear();
}
void FactTreeModel::itemDestroyed()
{
    conFactLayout.removeOne(qobject_cast<Fact *>(sender()));
    expandedFacts.removeAll(qobject_cast<Fact *>(sender()));
    //updateList.removeAll(QPointer<Fact>(static_cast<Fact*>(sender())));
}
//=============================================================================
//=============================================================================
