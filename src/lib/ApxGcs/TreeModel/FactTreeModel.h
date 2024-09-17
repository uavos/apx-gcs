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
#include <QtCore>

class FactTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit FactTreeModel(Fact *root, QObject *parent);
    ~FactTreeModel();

    Fact *fact(const QModelIndex &index) const;
    QModelIndex factIndex(FactBase *item, int column = 0) const;

    void recursiveDisconnect(Fact *fact);
    void checkConnections(Fact *fact) const;

    QList<QPointer<Fact>> expandedFacts;

    QHash<int, QByteArray> roleNames() const override;

    auto root() const { return _root; }
    void setRoot(Fact *f);

protected:
    Fact *_root{};

    //override
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private:
    QTimer updateTimer;
    QList<QPointer<Fact>> updateList;
    QMultiHash<Fact *, int> updateHash;

    QList<QPointer<Fact>> conFactLayout;

    using FactIndexHash = QMap<QPointer<Fact>, std::array<QModelIndex, Fact::FACT_MODEL_COLUMN_CNT>>;
    FactIndexHash indexHash;

private slots:
    void itemToBeInserted(int row, FactBase *item);
    void itemInserted(FactBase *);
    void itemToBeRemoved(int row, FactBase *item);
    void itemRemoved(FactBase *);
    void itemToBeMoved(int row, int dest, FactBase *item);
    void itemMoved(FactBase *item);

    void textChanged();
    void titleChanged();
    void descrChanged();
    void enabledChanged();
    void activeChanged();
    void modifiedChanged();
    void progressChanged();
    void visibleChanged();

    void updateTimerTimeout();
    void itemDestroyed();

    void indexHashCleanup();
};
