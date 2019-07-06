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
#include <Fact/Fact.h>
#include "NodeArrayListModel.h"
#include "NodesBase.h"
//=============================================================================
NodeArrayListModel::NodeArrayListModel(Fact *fact)
    : FactListModel(nullptr)
{
    setParent(fact);
    this->fact = fact;
    if (!fact)
        return;
    Fact *f1 = fact->child(0);
    for (int row = 0; row < f1->size(); ++row) {
        Fact *fi = f1->child(row);
        Fact *fRow = new Fact(nullptr, fi->name(), fi->title(), "", Fact::Group);
        fRow->setParent(this);
        fRow->setActionsModel(fact->actionsModel());
        //connect(fact,&Fact::destroyed,fRow,&Fact::deleteLater);
        //connect(fact,&Fact::destroyed,this,&NodeArrayListModel::sync);
        connect(fact, &Fact::modifiedChanged, fRow, [=]() { fRow->setModified(fact->modified()); });
        facts.append(fRow);
        connect(fi, &Fact::textChanged, fRow, [fRow, fi]() { fRow->setStatus(fi->text()); });
        connect(fi, &Fact::textChanged, fRow, [this, fRow]() { updateRowDescr(fRow); });

        Fact *f_ch = nullptr;
        for (int i = 0; i < fact->size(); ++i) {
            Fact *fArray = fact->child(i);
            if (!fArray)
                continue;
            Fact *fp;
            bool bChParam = false;
            if (f_ch && fArray->name().startsWith("ctr_ch_")) {
                fp = fArray->child(f_ch->value().toInt());
                bChParam = true;
            } else {
                fp = fArray->child(row);
            }
            if (!fp)
                continue;
            Fact *f = new Fact(fRow,
                               fArray->name(),
                               fArray->title(),
                               fArray->descr(),
                               fp->treeType() | fp->dataType());
            f->bind(fp);
            connect(f, &Fact::textChanged, fRow, [this, fRow]() { updateRowDescr(fRow); });
            //connect(fp,&Fact::removed,f,&Fact::deleteLater);
            //connect(fp,&Fact::destroyed,this,&NodeArrayListModel::sync);
            if (bChParam) {
                connect(f_ch, &Fact::valueChanged, f, [=]() {
                    f->bind(fArray->child(f_ch->value().toInt()));
                });
            }

            if (f->name() == "ctr_ch") {
                f_ch = f;
            }
        }
    }
    _items.clear();
    populate(&_items, fact);
}
//=============================================================================
void NodeArrayListModel::populate(ItemsList *list, Fact *fact)
{
    Q_UNUSED(fact)
    list->append(facts);
}
//=============================================================================
void NodeArrayListModel::updateRowDescr(Fact *fRow)
{
    QStringList st;
    if (!fRow->status().isEmpty()) {
        for (int i = 0; i < fRow->size(); ++i) {
            st.append(fRow->child(i)->text());
        }
    }
    fRow->setDescr(st.join(", "));
}
//=============================================================================
//=============================================================================
