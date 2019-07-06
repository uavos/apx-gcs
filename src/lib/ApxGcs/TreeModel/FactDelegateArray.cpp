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
#include "FactDelegateArray.h"
#include <Nodes/Nodes.h>
//=============================================================================
FactDelegateArray::FactDelegateArray(Fact *fact, QWidget *parent)
    : FactDelegateDialog(fact, parent)
{
    treeView = new QTreeView(this);
    treeView->setSizePolicy(
        QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

    treeView->setAlternatingRowColors(true);
    treeView->setRootIsDecorated(false);
    treeView->setEditTriggers(treeView->editTriggers() | QAbstractItemView::SelectedClicked);
    treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    treeView->header()->setMinimumSectionSize(50);
    treeView->setAutoFillBackground(false);

    model = new FactDelegateArrayModel(fact, this);
    treeView->setModel(model);
    treeView->setItemDelegate(new FactDelegate(this));

    setWidget(treeView);
}
//=============================================================================
//=============================================================================
FactDelegateArrayModel::FactDelegateArrayModel(Fact *group, QObject *parent)
    : QAbstractItemModel(parent)
    , group(group)
{
    bNodesGroup = qobject_cast<NodeItem *>(group->child(0));
    connect(group, &Fact::modifiedChanged, this, [=]() { emit layoutChanged(); });

    if (bNodesGroup) {
        //populate consolidated fnames list
        int nidx = -1;
        int fidx = 0;
        for (int i = 0; i < group->size(); ++i) {
            NodeItem *node = group->child<NodeItem>(i);
            nidx++;
            foreach (NodeField *f, node->allFields) {
                QString s = f->name();
                if (f->size()) {
                    //complex node field
                    //int cidx=0;
                    for (int j = 0; j < f->size(); ++j) {
                        NodeField *nfi = f->child<NodeField>(j);
                        QString sf;
                        //if(f->array()>1)sf=QString("%1/%2").arg(s).arg(QString::number(cidx++));
                        //else
                        sf = QString("%1/%2").arg(s).arg(nfi->title());
                        if (!fnames.contains(sf)) {
                            fnames.insert(fidx, sf);
                            fdescr.insert(sf, nfi->descr().isEmpty() ? f->descr() : nfi->descr());
                        }
                        fidx = fnames.indexOf(sf) + 1;
                        map.insert(QString("%1:%2").arg(sf).arg(nidx), nfi);
                    }
                    continue;
                }
                if (!fnames.contains(s)) {
                    fnames.insert(fidx, s);
                    fdescr.insert(s, f->descr());
                }
                fidx = fnames.indexOf(s) + 1;
                map.insert(QString("%1:%2").arg(s).arg(nidx), f);
            }
        }
    }
}
//=============================================================================
QVariant FactDelegateArrayModel::data(const QModelIndex &index, int role) const
{
    //check 'bind' column items
    bool binded = true;
    Fact *bind_item = group->child(0);
    if (bind_item && bind_item->name().endsWith("_bind")) {
        bind_item = bind_item->child(index.row());
        if (bind_item && bind_item->value().toInt() == 0)
            binded = false;
    }

    if (role == Qt::ForegroundRole) {
        if (index.column() > 0) {
            Fact *f = field(index);
            if (!f)
                return QVariant();
            if ((!binded) && (!f->modified())) {
                return QColor(Qt::darkGray);
            }
            return f->data(Fact::FACT_MODEL_COLUMN_VALUE, role);
        }
    }

    if (index.column() == 0) {
        while (1) {
            Fact *f = nullptr;
            if (!bNodesGroup) {
                f = group->child(0);
                if (f)
                    f = f->child(index.row());
                if (!f)
                    break;
                return f->data(Fact::FACT_MODEL_COLUMN_NAME, role);
            } else {
                if (role == Qt::DisplayRole)
                    return fnames.value(index.row());
                if (role == Qt::ToolTipRole)
                    return fdescr.value(fnames.value(index.row()));
                return QVariant();
            }
        }
        return QVariant();
    }

    Fact *f = field(index);
    if (!f)
        return QVariant();
    return f->data(Fact::FACT_MODEL_COLUMN_VALUE, role);
}
//=============================================================================
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
    //connect(f->field,SIGNAL(changed()),this,SIGNAL(layoutChanged()));
}
//=============================================================================
Fact *FactDelegateArrayModel::field(const QModelIndex &index) const
{
    Fact *f = qobject_cast<Fact *>(static_cast<QObject *>(index.internalPointer()));
    if (bNodesGroup)
        return f;
    //array type
    if (!f)
        return nullptr;
    int item_n = index.row();
    //check for controls array
    if (f->name().startsWith("ctr_")) {
        QString s = f->name();
        s = s.left(s.lastIndexOf('_'));
        //xx_yy_zz nested array
        if (s.contains('_')) {
            for (int i = 0; i < group->size(); ++i) {
                Fact *fi = group->child(i);
                if (!fi->name().startsWith(s))
                    continue;
                if ((int) item_n >= fi->size())
                    break; //protect
                fi = fi->child(item_n);
                if (fi)
                    item_n = fi->value().toInt();
                break;
            }
        }
    }
    if (item_n >= f->size())
        return nullptr;
    return f->child(item_n);
}
//=============================================================================
QModelIndex FactDelegateArrayModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    if (parent.isValid()) //root
        return QModelIndex();
    if (column < 1)
        return createIndex(row, column);
    if (map.isEmpty()) {
        return createIndex(row, column, group->child(column - 1));
    }
    return createIndex(row, column, map.value(QString("%1:%2").arg(fnames.at(row)).arg(column - 1)));
}
//=============================================================================
QModelIndex FactDelegateArrayModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}
//=============================================================================
int FactDelegateArrayModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    if (fnames.isEmpty()) {
        Fact *f = group->child(0);
        if (!f)
            return 0;
        return f->size();
    }
    return fnames.size();
}
//=============================================================================
int FactDelegateArrayModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (!group)
        return 0;
    return group->size() + 1;
}
//=============================================================================
QVariant FactDelegateArrayModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (!group)
        return QVariant();
    if (section == 0)
        return QVariant("#");
    Fact *f = group->child(section - 1);
    if (!f)
        return QVariant();
    if (role == Qt::DisplayRole) {
        QString s = f->title();
        if (bNodesGroup)
            return f->status().isEmpty()
                       ? s
                       : f->status(); //QString("%1 (%2)").arg(s).arg(f->status());
        return s.contains('_') ? s.mid(s.lastIndexOf('_') + 1) : s;
    }
    if (role == Qt::ToolTipRole)
        return group->child(section - 1)->descr();
    return QVariant();
}
//=============================================================================
Qt::ItemFlags FactDelegateArrayModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    const Qt::ItemFlags f0 = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    int column = index.column();
    if (column == 0 || (!field(index)))
        return f0;
    return f;
}
//=============================================================================
