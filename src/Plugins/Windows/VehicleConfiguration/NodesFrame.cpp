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
#include "NodesFrame.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <ApxMisc/MaterialIcon.h>
#include <ApxMisc/QActionFact.h>
#include <Nodes/Nodes.h>
#include <QAction>

#include <Database/DatabaseLookup.h>
//=============================================================================
NodesFrame::NodesFrame(QWidget *parent)
    : QWidget(parent)
    , vehicle(nullptr)
{
    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    vlayout->addWidget(toolBar);
    lbUavName = new QLabel(this);
    vlayout->addWidget(lbUavName);

    treeWidget = new FactTreeWidget(AppRoot::instance(), true, false, this);
    vlayout->addWidget(treeWidget);
    connect(treeWidget->tree,
            &FactTreeView::customContextMenuRequested,
            this,
            &NodesFrame::treeContextMenu);

    connect(treeWidget->tree->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &NodesFrame::updateActions);

    connect(Vehicles::instance(), &Vehicles::vehicleSelected, this, &NodesFrame::vehicleSelected);
    vehicleSelected(Vehicles::instance()->current());
}
//=============================================================================
void NodesFrame::vehicleSelected(Vehicle *v)
{
    if (vehicle)
        disconnect(vehicle);
    vehicle = v;
    Nodes *fNodes = v->f_nodes;
    treeWidget->setRoot(fNodes);
    lbUavName->setText(v->title());

    connect(vehicle->f_nodes, &Nodes::modifiedChanged, this, &NodesFrame::updateActions);

    foreach (QAction *a, toolBar->actions()) {
        toolBar->removeAction(a);
        a->deleteLater();
    }
    //make actions
    QAction *a;
    a = new QActionFact(vehicle->f_nodes->f_upload);
    toolBar->addAction(a);

    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_search));
    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_reload));
    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_stop));
    // toolBar->addAction(new QActionFact(vehicle->f_nodes->f_status));
    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_clear));

    QAction *aLookup = new QActionFact(vehicle->f_lookup);
    toolBar->addAction(aLookup);
    connect(aLookup, &QAction::triggered, treeWidget, &FactTreeWidget::resetFilter);

    QAction *aShare = new QActionFact(vehicle->f_share);
    toolBar->addAction(aShare);
    connect(aShare, &QAction::triggered, treeWidget, &FactTreeWidget::resetFilter);

    aUndo = toolBar->addAction(MaterialIcon("undo"),
                               tr("Revert"),
                               this,
                               &NodesFrame::aUndo_triggered);

    aDefaults = toolBar->addAction(MaterialIcon("lock-reset"),
                                   tr("Restore defaults"),
                                   this,
                                   &NodesFrame::aDefaults_triggered);

    //all actions signals
    foreach (QAction *a, toolBar->actions()) {
        toolBar->widgetForAction(a)->setObjectName(a->objectName());
        connect(a, &QAction::triggered, [this]() { treeWidget->tree->setFocus(); });
    }
    connect(vehicle->f_nodes->f_search, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(vehicle->f_nodes->f_reload, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    // connect(vehicle->f_nodes->f_status, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(vehicle->f_nodes->f_clear, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);

    updateActions();
}
//=============================================================================
void NodesFrame::updateActions(void)
{
    bool bMod = vehicle->f_nodes->modified();
    if (!bMod) {
        foreach (Fact *i, selectedItems()) {
            if (i->modified()) {
                bMod = true;
                break;
            }
        }
    }
    aUndo->setEnabled(bMod);
}
//=============================================================================
void NodesFrame::treeContextMenu(const QPoint &pos)
{
    //scan selected items
    QList<NodeItem *> nlist = selectedItems<NodeItem>();
    QMenu m(treeWidget);

    //editor actions
    QAction *a = new QAction(aDefaults->icon(), aDefaults->text(), &m);
    connect(a, &QAction::triggered, aDefaults, &QAction::trigger);
    m.addAction(a);
    for (auto i : selectedItems<Fact>()) {
        if (!i->modified())
            continue;
        a = new QAction(aUndo->icon(), aUndo->text(), &m);
        connect(a, &QAction::triggered, aUndo, &QAction::trigger);
        m.addAction(a);
        break;
    }
    m.addSeparator();

    //node tools
    for (auto node : nlist) {
        QString sect;
        for (int i = 0; i < node->tools->size(); ++i) {
            Fact *f = node->tools->child(i);
            if (sect.isEmpty())
                sect = f->section();
            else if (f->section() != sect && !m.isEmpty()) {
                m.addSeparator();
                sect = f->section();
            }
            addNodeTools(&m, f, node->title());
        }
    }
    if (nlist.size() > 1)
        updateMenuTitles(&m);

    if (!m.isEmpty())
        m.exec(treeWidget->tree->mapToGlobal(pos));
}
//=============================================================================
void NodesFrame::aUndo_triggered(void)
{
    treeWidget->tree->setFocus();
    QList<Fact *> list = selectedItems<Fact>();
    for (auto i : list) {
        if (i->modified())
            i->restore();
    }
    if (list.isEmpty())
        vehicle->f_nodes->restore();
}
void NodesFrame::aDefaults_triggered(void)
{
    treeWidget->tree->setFocus();
    QList<Fact *> list = selectedItems<Fact>();
    for (auto i : list) {
        i->restoreDefaults();
    }
    if (list.isEmpty())
        vehicle->f_nodes->restoreDefaults();
}
//=============================================================================
void NodesFrame::addNodeTools(QMenu *menu, Fact *fact, QString nodeName)
{
    if (fact->menu()) {
        if (fact->name() == "modules")
            return;

        //fact is group
        QMenu *m = nullptr;
        for (auto i : menu->findChildren<QMenu *>()) {
            if (i->objectName() == fact->title()) {
                m = i;
                break;
            }
        }
        if (!m) {
            m = menu->addMenu(fact->title());
            m->setObjectName(fact->title());
            m->setToolTip(nodeName);
        } else {
            //list nodes in tooltip
            QStringList st = m->toolTip().split(',', Qt::SkipEmptyParts);
            st.append(nodeName);
            st.removeDuplicates();
            m->setToolTip(st.join(','));
        }
        // add sub items

        DatabaseLookup *dbq = qobject_cast<DatabaseLookup *>(fact);
        if (dbq) {
            if (!m->actions().isEmpty()) {
                m->setEnabled(false);
                return;
            }
            dbq->defaultLookup(); //refresh db (delayed thread)
            int cnt = dbq->dbModel()->count();
            if (cnt > 30)
                cnt = 30;
            else if (cnt <= 0)
                m->setEnabled(false);
            for (int i = 0; i < cnt; ++i) {
                QVariantMap modelData = dbq->dbModel()->get(i);
                QString s = modelData.value("title").toString();
                QString s2 = modelData.value("value").toString();
                if (!s2.isEmpty())
                    s.append(QString(" (%1)").arg(s2));

                QAction *a = new QAction(s, m);
                a->setObjectName(s);
                a->setToolTip(nodeName);
                m->addAction(a);
                connect(a, &QAction::triggered, [dbq, modelData]() { dbq->triggerItem(modelData); });
            }
            return;
        }
        for (int i = 0; i < fact->size(); ++i) {
            addNodeTools(m, fact->child(i), nodeName);
        }
        return;
    }

    //fact is command
    QAction *a = nullptr;
    for (auto i : menu->actions()) {
        if (i->objectName() == fact->title()) {
            a = i;
            break;
        }
    }
    if (!a) {
        a = menu->addAction(fact->title());
        a->setObjectName(fact->title());
        a->setToolTip(nodeName);
    } else {
        //list nodes in tooltip
        QStringList st = a->toolTip().split(',', Qt::SkipEmptyParts);
        st.append(nodeName);
        st.removeDuplicates();
        a->setToolTip(st.join(','));
    }

    connect(a, &QAction::triggered, fact, [fact]() { fact->trigger(); });
}
void NodesFrame::updateMenuTitles(QMenu *menu)
{
    QStringList st = menu->toolTip().split(',', Qt::SkipEmptyParts);
    QString s = st.size() > 3 ? QString::number(st.size()) : st.join(',');
    menu->setTitle(QString("%1 [%2]").arg(menu->objectName()).arg(s));

    for (auto a : menu->actions()) {
        QStringList st = a->toolTip().split(',', Qt::SkipEmptyParts);
        QString s = st.size() > 3 ? QString::number(st.size()) : st.join(',');
        a->setText(QString("%1 [%2]").arg(a->objectName()).arg(s));
    }

    for (auto i : menu->findChildren<QMenu *>()) {
        updateMenuTitles(i);
    }
}
//=============================================================================
