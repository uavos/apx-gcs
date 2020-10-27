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

#include "FactDelegate.h"
#include "FactTreeModel.h"
#include "FactTreeView.h"

class FactDelegateMandalaModel : public FactTreeModel
{
    Q_OBJECT
public:
    FactDelegateMandalaModel(Fact *fact, QObject *parent);

protected:
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
};

class FactDelegateMandala : public QWidget
{
    Q_OBJECT
public:
    explicit FactDelegateMandala(Fact *fact, QWidget *parent);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    FactTreeView *tree;
    FactProxyModel *proxy;
    QLineEdit *eFilter;
    Fact *fact;

private slots:
    void updateFilter();
    void finished();
};
