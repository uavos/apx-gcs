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
#include "JSTreeView.h"
#include <ApxMisc/MaterialIcon.h>
#include <TreeModel/JSTreeModel.h>
#include <QHeaderView>
#include <QtWidgets>

JSTreeView::JSTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    //setSortingEnabled(true);
    //sortByColumn(JSTreeModel::JS_MODEL_COLUMN_NAME,Qt::AscendingOrder);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setDefaultSectionSize(50);
    header()->setMinimumSectionSize(70);
    header()->setVisible(false);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    //setItemDelegate(new FactDelegate(this));
    setUniformRowHeights(true);
    //setItemDelegate(new FactDelegate(this));

    setAlternatingRowColors(true);
    setUniformRowHeights(true);
    setAnimated(false);
    setIndentation(10);

    setFont(QGuiApplication::font());
}

JSTreeProxyModel::JSTreeProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_rootItem(nullptr)
{
    setDynamicSortFilter(false);
}

void JSTreeProxyModel::setRootItem(JSTreeItem *jsItem)
{
    m_rootItem = jsItem;
    invalidate();
}
JSTreeItem *JSTreeProxyModel::rootItem() const
{
    return m_rootItem;
}

bool JSTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow,
                                             JSTreeModel::JS_MODEL_COLUMN_NAME,
                                             sourceParent);
    JSTreeItem *jsItem = index.data(JSTreeModel::ModelDataRole).value<JSTreeItem *>();
    //if(!jsItem->visible())return false;
    if (!jsItem)
        return false;
    if (jsItem == m_rootItem)
        return true;
    //accept all parents of rootItem
    bool ok = false;
    for (JSTreeItem *f = m_rootItem; f; f = f->parentItem) {
        if (jsItem == f) {
            ok = true;
            break;
        }
    }
    //check if index has parent as rootindex
    if (!ok) {
        for (JSTreeItem *f = jsItem; f; f = f->parentItem) {
            if (f == m_rootItem) {
                //qDebug()<<"acc"<<jsItem->path();
                ok = true;
                break;
            }
        }
    }
    if (!ok) {
        //qDebug()<<"flt"<<jsItem->path();
        return filterRegularExpression().isValid() ? false : true;
    }
    return showThis(index);
}

bool JSTreeProxyModel::showThis(const QModelIndex index) const
{
    QModelIndex useIndex = sourceModel()->index(index.row(), 0, index.parent());

    bool bFlt = filterRegularExpression().isValid();
    JSTreeItem *item = useIndex.data(JSTreeModel::ModelDataRole).value<JSTreeItem *>();
    QRegularExpression re = filterRegularExpression();
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    if (bFlt && item) {
        if (item->parentItem != m_rootItem)
            return false;
        //if(item->showThis(re)==false)return false;
    }

    if (sourceModel()->data(useIndex, Qt::DisplayRole).toString().contains(re))
        return true;

    //look for matching parents
    /*QModelIndex parentIndex=sourceModel()->parent(index);
  while(parentIndex.isValid()){
    if(sourceModel()->data(parentIndex,Qt::DisplayRole).toString().contains(filterRegExp()))
      return true;
    parentIndex=sourceModel()->parent(parentIndex);
  }
  //look for matching childs
  for(int i=0;i<sourceModel()->rowCount(index);i++){
    QModelIndex childIndex=sourceModel()->index(i,JSTreeModel::JS_MODEL_COLUMN_NAME,index);
    if(!childIndex.isValid())break;
    if(showThis(childIndex))return true;
  }*/
    return false;
}

bool JSTreeProxyModel::lessThan2(const QModelIndex &left, const QModelIndex &right) const
{
    //only first col sorted
    if (left.column() != JSTreeModel::JS_MODEL_COLUMN_NAME
        || right.column() != JSTreeModel::JS_MODEL_COLUMN_NAME)
        return left.row() < right.row();
    JSTreeItem *item_left = left.data(JSTreeModel::ModelDataRole).value<JSTreeItem *>();
    JSTreeItem *item_right = right.data(JSTreeModel::ModelDataRole).value<JSTreeItem *>();
    if (!(item_left && item_right))
        return QSortFilterProxyModel::lessThan(left, right);
    return JSTreeItem::lessThan(item_left, item_right);
}

JSTreeWidget::JSTreeWidget(QJSEngine *e, bool filterEdit, bool backNavigation, QWidget *parent)
    : QWidget(parent)
{
    vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);
    tree = new JSTreeView(this);
    QSizePolicy sp = tree->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    tree->setSizePolicy(sp);
    eFilter = new QLineEdit(this);
    eFilter->setFrame(false);
    eFilter->setClearButtonEnabled(true);
    eFilter->setVisible(false);

    toolBar = new QToolBar(this);
    toolBar->setIconSize(QSize(14, 14));
    toolBar->layout()->setContentsMargins(0, 0, 0, 0);
    aBack = new QAction(MaterialIcon("arrow-left"), tr("Back"), this);
    aBack->setVisible(backNavigation);
    connect(aBack, &QAction::triggered, this, &JSTreeWidget::back);
    toolBar->addAction(aBack);
    if (backNavigation) {
        connect(tree, &JSTreeView::doubleClicked, this, &JSTreeWidget::doubleClicked);
    }
    lbPath = new QLabel(this);
    //lbPath->setVisible(false);
    toolBar->addWidget(lbPath);

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
    proxy = new JSTreeProxyModel(this);
    model = new JSTreeModel(e);
    proxy->setRootItem(model->root);
    proxy->setSourceModel(model);
    tree->setModel(proxy);

    updateActions();

    connect(eFilter, &QLineEdit::textChanged, this, &JSTreeWidget::filterChanged);
    connect(tree, &JSTreeView::collapsed, this, &JSTreeWidget::collapsed);
    connect(tree, &JSTreeView::expanded, this, &JSTreeWidget::expanded);
}

void JSTreeWidget::filterChanged()
{
    QString s = eFilter->text();
    auto regExp = QRegularExpression::fromWildcard(s,
                                                   Qt::CaseInsensitive,
                                                   QRegularExpression::UnanchoredWildcardConversion);
    proxy->setFilterRegularExpression(regExp);
    tree->setRootIndex(proxy->mapFromSource(model->itemIndex(proxy->rootItem())));
    updateActions();
    if (s.size()) {
        tree->expandAll();
    } else {
        tree->collapseAll();
        emit treeReset();
    }
}

void JSTreeWidget::doubleClicked(const QModelIndex &index)
{
    QModelIndex idx = proxy->mapToSource(index);
    JSTreeItem *f = model->item(idx);
    if (!f)
        return;
    if (!f->size())
        return;
    JSTreeItem *fPrev = proxy->rootItem();
    setRoot(f);
    rootList.append(QPointer<JSTreeItem>(fPrev));
    updateActions();
}

void JSTreeWidget::collapsed(const QModelIndex &index)
{
    Q_UNUSED(index)
    /*JSTreeItem *f=index.data(Fact::ModelDataRole).value<Fact*>();
  //if(f)model->recursiveDisconnect(f);
  model->expandedFacts.removeAll(f);*/
}
void JSTreeWidget::expanded(const QModelIndex &index)
{
    Q_UNUSED(index)
    /*JSTreeItem *f=proxy->mapToSource(index).data(Fact::ModelDataRole).value<Fact*>();
  if(!f)return;
  if(!model->expandedFacts.contains(f))model->expandedFacts.append(f);
  //qDebug()<<"exp"<<f->path();*/
}

void JSTreeWidget::updateActions()
{
    //JSTreeItem *f=tree->rootIndex().data(FactListModel::ModelDataRole).value<Fact*>();
    //rootList.removeAll(nullptr);
    bool bBack = !rootList.isEmpty();
    aBack->setEnabled(bBack);
    //qDebug()<<"upd";
}

void JSTreeWidget::resetFilter()
{
    tree->setFocus();
    eFilter->clear();
}

void JSTreeWidget::setRoot(JSTreeItem *jsItem)
{
    //if(!model->expandedFacts.contains(jsItem))model->expandedFacts.append(jsItem);
    resetFilter();
    proxy->setRootItem(jsItem);
    tree->setRootIndex(proxy->mapFromSource(model->itemIndex(jsItem)));
    //connect(jsItem,&Fact::removed,this,&JSTreeWidget::jsItemRemoved);
    //qDebug()<<"root"<<jsItem->path();
    //cut current path
    for (int i = 0; i < rootList.size(); i++) {
        JSTreeItem *f = rootList.at(i);
        if (f == nullptr || f == jsItem) {
            while (rootList.size() > i)
                rootList.removeLast();
            break;
        }
    }
    updateActions();
    lbPath->setText(jsItem->path());
}

void JSTreeWidget::back()
{
    //qDebug()<<"back";
    rootList.removeAll(nullptr);
    if (rootList.isEmpty())
        return;
    setRoot(rootList.last());
}

void JSTreeWidget::jsItemRemoved()
{
    /*resetFilter();
  if(rootList.isEmpty())return;
  JSTreeItem *jsItem=static_cast<Fact*>(sender());
  JSTreeItem *rootItem=nullptr;
  //qDebug()<<"removed"<<jsItem;
  for (int i=0;i<rootList.size();i++){
    JSTreeItem *f=rootList.at(i);
    if(f==nullptr || f==jsItem){
      if(rootItem)setRoot(rootItem);
      return;
    }
    rootItem=f;
  }
  if(jsItem==proxy->rootItem())back();*/
}
