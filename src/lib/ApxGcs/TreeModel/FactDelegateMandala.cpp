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
#include "FactDelegateMandala.h"
#include "FactTreeView.h"
#include <Mandala/Mandala.h>

FactDelegateMandala::FactDelegateMandala(Fact *fact, QWidget *parent)
    : QWidget(parent)
    , fact(fact)
{
    setWindowFlags(Qt::Popup);

    setWindowTitle(fact->title());
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    tree = new FactTreeView(this);
    QSizePolicy sp = tree->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
    sp.setVerticalPolicy(QSizePolicy::Preferred);
    tree->setSizePolicy(sp);
    //setSizePolicy(sp);
    eFilter = new QLineEdit(this);
    eFilter->setFrame(false);
    eFilter->setClearButtonEnabled(true);
    eFilter->setPlaceholderText(tr("Search").append("..."));

    vlayout->addWidget(eFilter);
    vlayout->addWidget(tree);

    //model
    proxy = new FactProxyModel(this);
    tree->setModel(proxy);
    FactDelegateMandalaModel *model = new FactDelegateMandalaModel(fact, this);
    proxy->setRootFact(fact->mandala());
    proxy->setSourceModel(model);

    connect(eFilter, &QLineEdit::textChanged, this, &FactDelegateMandala::updateFilter);

    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->expandToDepth(0);

    connect(tree, &FactTreeView::activated, this, [this](const QModelIndex &index) {
        QModelIndex idx = proxy->mapToSource(index);
        MandalaFact *f = qobject_cast<MandalaFact *>(idx.data(Fact::ModelDataRole).value<Fact *>());
        if (!f)
            return;
        if (!f->dataType())
            return;
        if (f->isSystem())
            return;
        close();
    });

    //find and select current item
    MandalaFact *mf = qobject_cast<Mandala *>(fact->mandala())->fact(fact->text());
    if (mf) {
        QVariant v = QVariant::fromValue(mf);
        QModelIndexList mlist = proxy->match(proxy->index(0, 0),
                                             Fact::ModelDataRole,
                                             v,
                                             1,
                                             Qt::MatchExactly | Qt::MatchRecursive);
        if (!mlist.isEmpty()) {
            tree->setCurrentIndex(mlist.first());
        }
    }

    //set geometry
    QRect scr = QApplication::desktop()->availableGeometry(parent);
    QPoint p = parent->mapToGlobal(QPoint(0, 0));
    setMaximumHeight(scr.height() - p.y());
    setMaximumWidth(scr.width() - p.x());
    move(p);
    show();
    QRect r = geometry();
    if (r.right() >= scr.width())
        move(r.x() - (r.right() - scr.width()), r.y());

    eFilter->setFocus(Qt::PopupFocusReason);
}
void FactDelegateMandala::closeEvent(QCloseEvent *event)
{
    event->accept();
    finished();
    deleteLater();
}
void FactDelegateMandala::updateFilter()
{
    QString s = eFilter->text();
    QRegExp regExp(s, Qt::CaseSensitive, QRegExp::WildcardUnix);
    proxy->setFilterRegExp(regExp);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    if (s.size()) {
        tree->expandAll();
    } else {
        tree->collapseAll();
        tree->expandToDepth(0);
    }
}
void FactDelegateMandala::finished()
{
    do {
        QModelIndexList sel = tree->selectionModel()->selectedRows();
        if (sel.isEmpty())
            break;
        QModelIndex idx = proxy->mapToSource(sel.first());
        MandalaFact *f = qobject_cast<MandalaFact *>(idx.data(Fact::ModelDataRole).value<Fact *>());
        if (!f)
            break;
        if (!f->dataType())
            break;
        if (f->isSystem())
            break;
        fact->setValue(f->mpath());
        return;
    } while (0);
    fact->setValue(QVariant());
}

FactDelegateMandalaModel::FactDelegateMandalaModel(Fact *fact, QObject *parent)
    : FactTreeModel(fact->mandala(), parent)
{}
Qt::ItemFlags FactDelegateMandalaModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags fx = FactTreeModel::flags(index);
    Fact *f = fact(index);
    if (!f)
        return fx;
    if (!f->dataType())
        fx &= ~Qt::ItemIsSelectable;
    return fx;
}
int FactDelegateMandalaModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return Fact::FACT_MODEL_COLUMN_CNT;
}
QVariant FactDelegateMandalaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    Fact *f = fact(index);
    if (!f)
        return QVariant();
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Fact::FACT_MODEL_COLUMN_NAME:
            return f->title();
        case Fact::FACT_MODEL_COLUMN_VALUE:
            if (f->treeType() != Fact::Group)
                break;
            return QVariant();
        case Fact::FACT_MODEL_COLUMN_DESCR:
            if (MandalaFact *m = qobject_cast<MandalaFact *>(f))
                return m->mpath();
            break;
        }
    }
    return FactTreeModel::data(index, role);
}
