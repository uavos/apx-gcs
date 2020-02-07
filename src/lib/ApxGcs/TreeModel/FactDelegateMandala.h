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
