#include "FactTreeView.h"
#include "FactDelegate.h"
#include "FactTreeModel.h"
#include <ApxMisc/MaterialIcon.h>
#include <QHeaderView>
#include <QtWidgets>
//=============================================================================
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
}
//=============================================================================
//=============================================================================
FactProxyModel::FactProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_rootFact(nullptr)
{
    setDynamicSortFilter(false);
}
//=============================================================================
void FactProxyModel::setRootFact(Fact *fact)
{
    m_rootFact = fact;
    invalidate();
}
Fact *FactProxyModel::rootFact() const
{
    return m_rootFact;
}
//=============================================================================
bool FactProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, Fact::FACT_MODEL_COLUMN_NAME, sourceParent);
    Fact *fact = index.data(Fact::ModelDataRole).value<Fact *>();
    if (!fact)
        return false;
    if (!fact->visible())
        return false;
    if (fact == m_rootFact)
        return true;
    //accept all parents of rootFact
    bool ok = false;
    for (Fact *f = m_rootFact; f; f = f->parentFact()) {
        if (fact == f) {
            ok = true;
            break;
        }
    }
    //check if index has parent as rootindex
    if (!ok) {
        for (Fact *f = fact; f; f = f->parentFact()) {
            if (f == m_rootFact) {
                //qDebug()<<"acc"<<fact->path();
                ok = true;
                break;
            }
        }
    }
    if (!ok) {
        //qDebug()<<"flt"<<fact->path();
        return filterRegExp().isEmpty() ? true : false;
    }
    return showThis(index);
}
//=============================================================================
bool FactProxyModel::showThis(const QModelIndex index) const
{
    if (showThisItem(index))
        return true;
    //look for matching parents
    QModelIndex parentIndex = sourceModel()->parent(index);
    while (parentIndex.isValid()) {
        if (showThisItem(parentIndex))
            return true;
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
        return f->showThis(filterRegExp());
    return false;
}
//=============================================================================
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
    return item_left->lessThan(item_right);
}
//=============================================================================
//=============================================================================
FactTreeWidget::FactTreeWidget(Fact *fact, bool filterEdit, bool backNavigation, QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(fact->title());
    vlayout = new QVBoxLayout(this);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    tree = new FactTreeView(this);
    QSizePolicy sp = tree->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    tree->setSizePolicy(sp);
    eFilter = new QLineEdit(this);
    eFilter->setFrame(false);
    eFilter->setClearButtonEnabled(true);
    eFilter->setVisible(false);

    toolBar = new QToolBar(this);
    //toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setIconSize(QSize(14, 14));
    toolBar->layout()->setMargin(0);
    aBack = new QAction(MaterialIcon("arrow-left"), tr("Back"), this);
    aBack->setVisible(backNavigation);
    connect(aBack, &QAction::triggered, this, &FactTreeWidget::back);
    toolBar->addAction(aBack);
    if (backNavigation) {
        connect(tree, &FactTreeView::doubleClicked, this, &FactTreeWidget::doubleClicked);
    }

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
    proxy = new FactProxyModel(this);
    tree->setModel(proxy);
    model = new FactTreeModel(fact);
    proxy->setRootFact(fact);
    proxy->setSourceModel(model);

    updateActions();

    connect(eFilter, &QLineEdit::textChanged, this, &FactTreeWidget::filterChanged);
    connect(tree, &FactTreeView::collapsed, this, &FactTreeWidget::collapsed);
    connect(tree, &FactTreeView::expanded, this, &FactTreeWidget::expanded);
}
//=============================================================================
void FactTreeWidget::filterChanged()
{
    QString s = eFilter->text();
    QRegExp regExp(s, Qt::CaseSensitive, QRegExp::WildcardUnix);
    //QModelIndex rootIndex=tree->rootIndex();
    proxy->setFilterRegExp(regExp);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    tree->setRootIndex(proxy->mapFromSource(model->factIndex(proxy->rootFact())));
    updateActions();
    if (s.size()) {
        tree->expandAll();
    } else {
        tree->collapseAll();
        emit treeReset();
    }
}
//=============================================================================
void FactTreeWidget::doubleClicked(const QModelIndex &index)
{
    QModelIndex idx = proxy->mapToSource(index);
    Fact *f = idx.data(Fact::ModelDataRole).value<Fact *>();
    if (!f)
        return;
    if (!f->size())
        return;
    Fact *fPrev = proxy->rootFact();
    setRoot(f);
    rootList.append(QPointer<Fact>(fPrev));
    updateActions();
}
//=============================================================================
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
//=============================================================================
void FactTreeWidget::updateActions()
{
    //Fact *f=tree->rootIndex().data(FactListModel::ModelDataRole).value<Fact*>();
    //rootList.removeAll(nullptr);
    bool bBack = !rootList.isEmpty();
    aBack->setEnabled(bBack);
    //qDebug()<<"upd";
}
//=============================================================================
void FactTreeWidget::resetFilter()
{
    tree->setFocus();
    eFilter->clear();
}
//=============================================================================
void FactTreeWidget::setRoot(Fact *fact)
{
    if (!model->expandedFacts.contains(fact))
        model->expandedFacts.append(fact);
    resetFilter();
    //if(proxy->rootFact()) disconnect(proxy->rootFact(),&Fact::removed,this,&FactTreeWidget::factRemoved);
    proxy->setRootFact(fact);
    tree->setRootIndex(proxy->mapFromSource(model->factIndex(fact)));
    connect(fact, &Fact::removed, this, &FactTreeWidget::factRemoved);
    //qDebug()<<"root"<<fact->path();
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
//=============================================================================
void FactTreeWidget::back()
{
    //qDebug()<<"back";
    rootList.removeAll(nullptr);
    if (rootList.isEmpty())
        return;
    setRoot(rootList.last());
}
//=============================================================================
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
    if (fact == proxy->rootFact())
        back();
}
//=============================================================================
