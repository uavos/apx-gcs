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
#pragma once

#include <Fact/Fact.h>
#include <QStyledItemDelegate>
#include <QtCore>
#include <QtWidgets>
class Fact;
class FactTreeModel;

class FactTreeView : public QTreeView
{
    Q_OBJECT
public:
    FactTreeView(QWidget *parent = nullptr);

    QSize sizeHint() const override { return viewportSizeHint(); }

    template<class T = Fact>
    inline QList<T *> selectedItems() const
    {
        if (!selectionModel())
            return {};
        QList<T *> list;
        for (auto index : selectionModel()->selectedRows()) {
            auto t = qobject_cast<T *>(index.data(Fact::ModelDataRole).value<Fact *>());
            if (t)
                list.append(t);
        }
        return list;
    }
};

class FactProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit FactProxyModel(QObject *parent = nullptr);

    void setRoot(Fact *f);
    auto root() const { return _root; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    virtual bool showThisFact(Fact *f) const;
    virtual bool lessThan(Fact *left, Fact *right) const;

private:
    QPointer<Fact> _root{};
    bool showThis(const QModelIndex index) const;
    bool showThisItem(const QModelIndex index) const;
};

class FactTreeWidget : public QWidget
{
    Q_OBJECT
public:
    FactTreeWidget(Fact *fact, bool filterEdit, bool backNavigation, QWidget *parent = nullptr);

    FactTreeView *tree;
    FactProxyModel *proxy;
    FactTreeModel *model;
    QLineEdit *eFilter;
    QAction *aBack;
    QToolBar *toolBar;
    QVBoxLayout *vlayout;

    auto root() const { return proxy->root(); }

private:
    QList<QPointer<Fact>> rootList;
    bool _backNavigation{};

private slots:
    void filterChanged();
    void doubleClicked(const QModelIndex &index);
    void collapsed(const QModelIndex &index);
    void expanded(const QModelIndex &index);
    void updateActions();
    void factRemoved();

public slots:
    void resetFilter();
    void back();
    void setRoot(Fact *fact);

signals:
    void treeReset();
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
};
