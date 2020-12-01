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
#include "FactDelegateArray.h"
#include <Nodes/Nodes.h>

FactDelegateArray::FactDelegateArray(Fact *fact, QWidget *parent)
    : FactDelegateDialog(fact, parent)
{
    treeView = new QTreeView(this);
    treeView->setSizePolicy(
        QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

    treeView->setAlternatingRowColors(true);
    treeView->setRootIsDecorated(false);
    treeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    //treeView->setEditTriggers(treeView->editTriggers() | QAbstractItemView::SelectedClicked);
    treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    treeView->header()->setMinimumSectionSize(50);
    treeView->setAutoFillBackground(false);

    model = new FactDelegateArrayModel(fact->menu(), this);
    treeView->setModel(model);
    treeView->setItemDelegate(new FactDelegate(this));

    setWidget(treeView);
}

FactDelegateArrayModel::FactDelegateArrayModel(Fact *group, QObject *parent)
    : QAbstractItemModel(parent)
    , group(group)
{
    connect(group, &Fact::modifiedChanged, this, [this]() { emit layoutChanged(); });
}

QVariant FactDelegateArrayModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == 0) {
        Fact *f = group->child(index.row());
        if (!f)
            return QVariant();
        return f->data(Fact::FACT_MODEL_COLUMN_NAME, role);
    }

    Fact *f = field(index);
    if (!f)
        return QVariant();

    if (role == Qt::ForegroundRole && !f->modified()) {
        Fact *bind_item = group->child(index.row()) ? group->child(index.row())->child(0) : nullptr;
        if (bind_item && bind_item->name() == "bind" && bind_item->isZero()) {
            return QColor(Qt::darkGray);
        }
    }

    return f->data(Fact::FACT_MODEL_COLUMN_VALUE, role);
}

bool FactDelegateArrayModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;
    Fact *f = field(index);
    if (!f)
        return false;
    if (!f->setValue(value))
        return false;
    emit layoutChanged();
    return true;
}

Fact *FactDelegateArrayModel::field(const QModelIndex &index) const
{
    return qobject_cast<Fact *>(static_cast<QObject *>(index.internalPointer()));
}

QModelIndex FactDelegateArrayModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    if (parent.isValid()) //root
        return QModelIndex();
    if (column < 1)
        return createIndex(row, column);

    Fact *f = group->child(row);
    if (!f)
        return QModelIndex();

    return createIndex(row, column, f->child(column - 1));
}

QModelIndex FactDelegateArrayModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int FactDelegateArrayModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return group->size();
}

int FactDelegateArrayModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (!group || !group->child(0))
        return 0;
    return group->child(0)->size() + 1;
}

QVariant FactDelegateArrayModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (!group || !group->child(0))
        return QVariant();
    if (section == 0)
        return QVariant("#");
    Fact *f = group->child(0)->child(section - 1);
    if (!f)
        return QVariant();

    if (role == Qt::DisplayRole)
        return f->title();

    if (role == Qt::ToolTipRole)
        return f->descr();

    return QVariant();
}

Qt::ItemFlags FactDelegateArrayModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    const Qt::ItemFlags f0 = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    auto fact = field(index);
    int column = index.column();
    if (column == 0 || (!fact))
        return f0;
    if (!fact->enabled())
        return f0;

    return f;
}
