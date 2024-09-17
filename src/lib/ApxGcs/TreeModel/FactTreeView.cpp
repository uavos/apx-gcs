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
#include "FactTreeView.h"
#include "FactDelegate.h"
#include "FactTreeModel.h"
#include <ApxMisc/MaterialIcon.h>
#include <QHeaderView>
#include <QtWidgets>

FactTreeView::FactTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);
    sortByColumn(Fact::FACT_MODEL_COLUMN_NAME, Qt::AscendingOrder);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setDefaultSectionSize(50);
    header()->setMinimumSectionSize(70);
    header()->setVisible(false);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    //setItemDelegate(new FactDelegate(this));
    setUniformRowHeights(true);
    setItemDelegate(new FactDelegate(this));

    setAlternatingRowColors(true);
    setUniformRowHeights(true);
    setAnimated(false);
    setIndentation(10);

    setFont(QGuiApplication::font());
}

FactProxyModel::FactProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);
}

void FactProxyModel::setRoot(Fact *f)
{
    beginResetModel();
    _root = f;
    endResetModel();

    invalidate();
}

bool FactProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, Fact::FACT_MODEL_COLUMN_NAME, sourceParent);
    Fact *fact = index.data(Fact::ModelDataRole).value<Fact *>();
    if (!fact)
        return false;

    if (!fact->visible())
        return false;

    if (fact == _root)
        return true;

    //accept all parents of rootFact
    bool ok = false;
    for (Fact *f = _root; f; f = f->parentFact()) {
        if (fact == f) {
            ok = true;
            break;
        }
    }

    //check if index has parent as rootindex
    if (!ok) {
        for (Fact *f = fact; f; f = f->parentFact()) {
            if (f == _root) {
                //qDebug()<<"acc"<<fact->path();
                ok = true;
                break;
            }
        }
    }

    if (!ok) {
        //qDebug()<<"flt"<<fact->path();
        return filterRegularExpression().isValid() ? false : true;
    }

    return showThis(index);
}

bool FactProxyModel::showThis(const QModelIndex index) const
{
    if (showThisItem(index))
        return true;

    //look for matching parents
    QModelIndex parentIndex = sourceModel()->parent(index);
    while (parentIndex.isValid()) {
        if (showThisItem(parentIndex)) {
            return true;
        }
        parentIndex = sourceModel()->parent(parentIndex);
    }
    //look for matching childs
    for (int i = 0; i < sourceModel()->rowCount(index); i++) {
        QModelIndex childIndex = sourceModel()->index(i, Fact::FACT_MODEL_COLUMN_NAME, index);
        if (!childIndex.isValid())
            break;
        if (showThis(childIndex))
            return true;
    }
    return false;
}
bool FactProxyModel::showThisItem(const QModelIndex index) const
{
    Fact *f = sourceModel()->data(index, Fact::ModelDataRole).value<Fact *>();
    if (f)
        return showThisFact(f);
    return false;
}
bool FactProxyModel::showThisFact(Fact *f) const
{
    return f->showThis(filterRegularExpression());
}

bool FactProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    //only first col sorted
    if (left.column() != Fact::FACT_MODEL_COLUMN_NAME
        || right.column() != Fact::FACT_MODEL_COLUMN_NAME)
        return left.row() < right.row();
    Fact *item_left = left.data(Fact::ModelDataRole).value<Fact *>();
    Fact *item_right = right.data(Fact::ModelDataRole).value<Fact *>();
    if (!(item_left && item_right))
        return QSortFilterProxyModel::lessThan(left, right);
    return lessThan(item_left, item_right);
}
bool FactProxyModel::lessThan(Fact *left, Fact *right) const
{
    return left->lessThan(right);
}

FactTreeWidget::FactTreeWidget(Fact *fact, bool filterEdit, bool backNavigation, QWidget *parent)
    : QWidget(parent)
    , _backNavigation(backNavigation)
{
    vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);
    tree = new FactTreeView(this);
    QSizePolicy sp = tree->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    tree->setSizePolicy(sp);
    eFilter = new QLineEdit(this);
    eFilter->setFrame(false);
    eFilter->setClearButtonEnabled(true);
    eFilter->setVisible(false);
    eFilter->setPlaceholderText(tr("Search").append("..."));

    toolBar = new QToolBar(this);
    //toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setIconSize(QSize(14, 14));
    toolBar->layout()->setContentsMargins(0, 0, 0, 0);
    aBack = new QAction(MaterialIcon("arrow-left"), tr("Back"), this);
    aBack->setVisible(backNavigation);
    connect(aBack, &QAction::triggered, this, &FactTreeWidget::back);
    toolBar->addAction(aBack);

    connect(tree, &FactTreeView::doubleClicked, this, &FactTreeWidget::doubleClicked);

    vlayout->addWidget(toolBar);
    vlayout->addWidget(tree);

    if (filterEdit) {
        eFilter->setVisible(true);
        toolBar->addWidget(eFilter);
        if (!backNavigation) {
            //move toolBar to bottom
            vlayout->removeWidget(toolBar);
            vlayout->addWidget(toolBar);
        }
    }
    toolBar->setVisible(backNavigation || filterEdit);

    //model
    model = new FactTreeModel(fact, this);
    proxy = new FactProxyModel(this);
    proxy->setSourceModel(model);
    tree->setModel(proxy);

    setRoot(fact);

    connect(eFilter,
            &QLineEdit::textChanged,
            this,
            &FactTreeWidget::filterChanged,
            Qt::QueuedConnection);

    connect(tree, &FactTreeView::collapsed, this, &FactTreeWidget::collapsed);
    connect(tree, &FactTreeView::expanded, this, &FactTreeWidget::expanded);
}

void FactTreeWidget::filterChanged()
{
    QString s = eFilter->text().trimmed();
    auto regExp = QRegularExpression::fromWildcard(s,
                                                   Qt::CaseInsensitive,
                                                   QRegularExpression::UnanchoredWildcardConversion);

    auto rootIndex = tree->rootIndex();
    //qDebug() << rootIndex;
    // tree->reset();

    proxy->setFilterRegularExpression(regExp);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // tree->setRootIndex(proxy->mapFromSource(model->factIndex(proxy->rootFact())));
    tree->setRootIndex(rootIndex);
    tree->reset();

    updateActions();
    if (s.size()) {
        tree->expandAll();
    } else {
        tree->collapseAll();
        emit treeReset();
    }
}

void FactTreeWidget::doubleClicked(const QModelIndex &index)
{
    QModelIndex idx = proxy->mapToSource(index);
    Fact *f = idx.data(Fact::ModelDataRole).value<Fact *>();
    if (!f)
        return;

    if (f->size() > 0 || f->treeType() == Fact::Group) {
        if (!_backNavigation)
            return;
        Fact *fPrev = proxy->root();
        setRoot(f);
        rootList.append(QPointer<Fact>(fPrev));
        updateActions();
        return;
    }

    // value fact
    f->trigger();
}

void FactTreeWidget::collapsed(const QModelIndex &index)
{
    Fact *f = index.data(Fact::ModelDataRole).value<Fact *>();
    //if(f)model->recursiveDisconnect(f);
    model->expandedFacts.removeAll(f);
}
void FactTreeWidget::expanded(const QModelIndex &index)
{
    Fact *f = proxy->mapToSource(index).data(Fact::ModelDataRole).value<Fact *>();
    if (!f)
        return;
    if (!model->expandedFacts.contains(f))
        model->expandedFacts.append(f);
    //qDebug()<<"exp"<<f->path();
}

void FactTreeWidget::updateActions()
{
    //Fact *f=tree->rootIndex().data(FactListModel::ModelDataRole).value<Fact*>();
    //rootList.removeAll(nullptr);
    bool bBack = !rootList.isEmpty();
    aBack->setEnabled(bBack);
    //qDebug()<<"upd";
}

void FactTreeWidget::resetFilter()
{
    tree->setFocus();
    eFilter->clear();
}

void FactTreeWidget::setRoot(Fact *fact)
{
    if (!model->expandedFacts.contains(fact))
        model->expandedFacts.append(fact);

    resetFilter();

    if (model->root()) {
        model->root()->disconnect(this);

        if (tree->selectionModel()) {
            tree->selectionModel()->disconnect(this);
        }
    }

    model->setRoot(fact);
    proxy->setRoot(fact);
    proxy->invalidate();

    if (fact) {
        setWindowTitle(fact->title());

        connect(fact, &Fact::removed, this, &FactTreeWidget::factRemoved);
        connect(tree->selectionModel(),
                &QItemSelectionModel::selectionChanged,
                this,
                &FactTreeWidget::selectionChanged);
    }

    //cut current path
    for (int i = 0; i < rootList.size(); i++) {
        Fact *f = rootList.at(i);
        if (f == nullptr || f == fact) {
            while (rootList.size() > i)
                rootList.removeLast();
            break;
        }
    }

    updateActions();
}

void FactTreeWidget::back()
{
    //qDebug()<<"back";
    rootList.removeAll(nullptr);
    if (rootList.isEmpty())
        return;
    setRoot(rootList.last());
}

void FactTreeWidget::factRemoved()
{
    resetFilter();
    if (rootList.isEmpty())
        return;
    Fact *fact = qobject_cast<Fact *>(sender());
    Fact *rootFact = nullptr;
    //qDebug()<<"removed"<<fact;
    for (int i = 0; i < rootList.size(); i++) {
        Fact *f = rootList.at(i);
        if (f == nullptr || f == fact) {
            if (rootFact)
                setRoot(rootFact);
            return;
        }
        rootFact = f;
    }
    if (fact == proxy->root())
        back();
}
