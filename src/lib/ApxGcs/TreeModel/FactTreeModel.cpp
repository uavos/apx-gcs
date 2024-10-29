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
#include "FactTreeModel.h"
#include <Fact/Fact.h>
#include <QDomDocument>

FactTreeModel::FactTreeModel(Fact *root, QObject *parent)
    : QAbstractItemModel(parent)
{
    updateTimer.setSingleShot(true);
    updateTimer.setInterval(500);
    connect(&updateTimer, &QTimer::timeout, this, &FactTreeModel::updateTimerTimeout);

    setRoot(root);
}

FactTreeModel::~FactTreeModel() {}

void FactTreeModel::setRoot(Fact *f)
{
    updateList.clear();

    beginResetModel();

    for (auto &i : conFactLayout)
        recursiveDisconnect(i);

    _root = f;
    endResetModel();

    checkConnections(f);
    indexHashCleanup();
}

QHash<int, QByteArray> FactTreeModel::roleNames() const
{
    return _root->model()->roleNames();
}

QVariant FactTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    auto f = fact(index);
    if (!f)
        return {};
    return f->data(index.column(), role);
}

bool FactTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ((!index.isValid()) || (role != Qt::EditRole)
        || index.column() != Fact::FACT_MODEL_COLUMN_VALUE)
        return false;
    auto f = fact(index);
    if (!f)
        return false;
    if (data(index, role) == value)
        return true;
    bool rv = f->setValue(value);
    updateTimerTimeout();
    //if(rv)emit dataChanged(index,index);//layoutChanged();
    return rv;
}

QModelIndex FactTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    auto parentFact = parent.isValid() ? fact(parent) : _root;
    if (!parentFact)
        return {};

    auto f = parentFact->child(row);
    if (!f)
        return {};

    checkConnections(f);
    auto i = createIndex(row, column, f);
    (*const_cast<FactIndexHash *>(&indexHash))[f][column] = i;
    return i;
}

QModelIndex FactTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto f = fact(index);
    if (!f)
        return {};

    auto p = f->parentFact();
    if (!p || p == _root || !p->parentFact())
        return {};

    int row = p->indexInParent();
    if (row < 0)
        return {};

    checkConnections(p);
    auto i = createIndex(row, 0, p);
    (*const_cast<FactIndexHash *>(&indexHash))[p][0] = i;
    return i;
}

int FactTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    auto parentFact = parent.isValid() ? fact(parent) : _root;
    return parentFact ? parentFact->size() : 0;
}

int FactTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return Fact::FACT_MODEL_COLUMN_CNT;
}

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

Qt::ItemFlags FactTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags fx = QAbstractItemModel::flags(index);
    auto f = fact(index);
    if (!f)
        return fx;
    if (f->enabled())
        fx |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return fx;
    if (index.column() != Fact::FACT_MODEL_COLUMN_VALUE)
        return fx;
    if (f->treeType() == Fact::Group && f->size() > 1 && f->child(0)->treeType() == Fact::Group
        && f->value().toString().startsWith('[') && f->value().toString().endsWith(']'))
        return fx | Qt::ItemIsEditable; //array editor

    if (f->treeType())
        return fx;
    if (f->dataType() && f->dataType() != Fact::Count) {
        fx |= Qt::ItemIsEditable;
    }
    return fx;
}

Fact *FactTreeModel::fact(const QModelIndex &index) const
{
    return qobject_cast<Fact *>(static_cast<QObject *>(index.internalPointer()));
}
QModelIndex FactTreeModel::factIndex(FactBase *item, int column) const
{
    if (!item || item == _root)
        return {};

    auto f = qobject_cast<Fact *>(item);
    if (!f)
        return {};

    return (*const_cast<FactIndexHash *>(&indexHash))[f][column];
}

void FactTreeModel::indexHashCleanup()
{
    // remove deleted facts

    // qDebug() << "\n\n\n\n" << indexHash.size();

    if (indexHash.contains(nullptr))
        return;

    size_t rm = 0;
    for (;;) {
        auto cnt = indexHash.remove(nullptr);
        if (cnt <= 0)
            break;
        rm += cnt;
    }
    // qDebug() << "\n\n\nindex removed:" << rm << "\n\n";
}

void FactTreeModel::checkConnections(Fact *fact) const
{
    if (!fact || conFactLayout.contains(fact))
        return;

    const_cast<FactTreeModel *>(this)->indexHashCleanup();

    const_cast<QList<QPointer<Fact>> *>(&conFactLayout)->append(fact);

    Qt::ConnectionType t;
    t = (Qt::ConnectionType)(Qt::UniqueConnection);
    connect(fact, &Fact::removed, this, &FactTreeModel::itemDestroyed, t);
    connect(fact, &Fact::destroyed, this, &FactTreeModel::itemDestroyed, t);

    t = (Qt::ConnectionType)(Qt::UniqueConnection | Qt::QueuedConnection);
    connect(fact, &Fact::itemToBeInserted, this, &FactTreeModel::itemToBeInserted, t);
    connect(fact, &Fact::itemInserted, this, &FactTreeModel::itemInserted, t);
    connect(fact, &Fact::itemToBeRemoved, this, &FactTreeModel::itemToBeRemoved, t);
    connect(fact, &Fact::itemRemoved, this, &FactTreeModel::itemRemoved, t);
    connect(fact, &Fact::itemToBeMoved, this, &FactTreeModel::itemToBeMoved, t);
    connect(fact, &Fact::itemMoved, this, &FactTreeModel::itemMoved, t);

    t = (Qt::ConnectionType)(Qt::UniqueConnection | Qt::QueuedConnection);
    connect(fact, &Fact::textChanged, this, &FactTreeModel::textChanged, t);
    connect(fact, &Fact::titleChanged, this, &FactTreeModel::titleChanged, t);
    connect(fact, &Fact::descrChanged, this, &FactTreeModel::descrChanged, t);
    connect(fact, &Fact::enabledChanged, this, &FactTreeModel::enabledChanged, t);
    connect(fact, &Fact::activeChanged, this, &FactTreeModel::activeChanged, t);
    connect(fact, &Fact::modifiedChanged, this, &FactTreeModel::modifiedChanged, t);
    connect(fact, &Fact::progressChanged, this, &FactTreeModel::progressChanged, t);
    connect(fact, &Fact::visibleChanged, this, &FactTreeModel::visibleChanged, t);
}

void FactTreeModel::recursiveDisconnect(Fact *fact)
{
    if (!qobject_cast<Fact *>(fact)) {
        conFactLayout.removeAll(fact);
        return;
    }

    if (!conFactLayout.contains(fact))
        return;

    fact->disconnect(this);
    indexHash.remove(fact);

    //resetInternalData();
    conFactLayout.removeAll(fact);

    for (auto f : fact->facts()) {
        recursiveDisconnect(f);
    }
}

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

void FactTreeModel::textChanged()
{
    if (!sender())
        return;

    auto f = qobject_cast<Fact *>(sender())->parentFact();
    if (!f)
        return;
    //qDebug()<<fact->path();
    if (!updateList.contains(f))
        updateList.append(f);
    if (!updateHash.values(f).contains(Fact::FACT_MODEL_COLUMN_VALUE))
        updateHash.insert(f, Fact::FACT_MODEL_COLUMN_VALUE);
    if (!updateTimer.isActive())
        updateTimer.start();
}
void FactTreeModel::titleChanged()
{
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    if (index.isValid())
        emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
}
void FactTreeModel::descrChanged()
{
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_DESCR);
    if (index.isValid())
        emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
}
void FactTreeModel::enabledChanged()
{
    QModelIndex index1 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    QModelIndex index2 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_DESCR);
    if (index1.isValid() && index2.isValid())
        emit dataChanged(index1, index2, QVector<int>() << Qt::ForegroundRole << Qt::BackgroundRole);
}
void FactTreeModel::activeChanged()
{
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    if (index.isValid())
        emit dataChanged(index, index, QVector<int>() << Qt::ForegroundRole);
}
void FactTreeModel::modifiedChanged()
{
    QModelIndex index1 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_NAME);
    QModelIndex index2 = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_VALUE);
    if (index1.isValid() && index2.isValid())
        emit dataChanged(index1, index2, QVector<int>() << Qt::ForegroundRole);
}
void FactTreeModel::progressChanged()
{
    Fact *fact = qobject_cast<Fact *>(sender());
    if (!fact)
        return;
    if (!expandedFacts.contains(fact->parentFact()))
        return;
    QModelIndex index = factIndex(qobject_cast<Fact *>(sender()), Fact::FACT_MODEL_COLUMN_DESCR);
    emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
}
void FactTreeModel::visibleChanged()
{
    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

void FactTreeModel::updateTimerTimeout()
{
    updateTimer.stop();
    for (auto &f : updateList) {
        if (f.isNull())
            continue;
        if (f->size() <= 0)
            continue;
        if (!expandedFacts.contains(f))
            continue;
        for (auto col : updateHash.values(f)) {
            QModelIndex index1 = factIndex(f->child(0), col);
            QModelIndex index2 = factIndex(f->child(f->size() - 1), col);
            if (index1.isValid() && index2.isValid())
                emit dataChanged(index1, index2, QVector<int>() << Qt::DisplayRole);
            //qDebug()<<f->path();
        }
    }
    updateList.clear();
    updateHash.clear();
}
void FactTreeModel::itemDestroyed()
{
    auto f = qobject_cast<Fact *>(sender());
    conFactLayout.removeAll(f);
    expandedFacts.removeAll(f);
    updateList.removeAll(f);
    indexHash.remove(f);
}
