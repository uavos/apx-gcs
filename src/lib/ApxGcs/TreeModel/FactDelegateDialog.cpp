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
#include "FactDelegateDialog.h"
#include <ApxMisc/MaterialIcon.h>
#include <Nodes/Nodes.h>
//=============================================================================
QHash<Fact *, FactDelegateDialog *> FactDelegateDialog::dlgMap;
//=============================================================================
FactDelegateDialog::FactDelegateDialog(Fact *fact, QWidget *parent)
    : QDialog(parent)
    , fact(fact)
    , widget(nullptr)
{
    setObjectName(fact->title());
    //setWindowFlags(windowFlags()|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
    setWindowTitle((fact->descr().size() ? fact->descr() : fact->title())
                   + QString(" (%1)").arg(fact->titlePath()));

    vlayout = new QVBoxLayout(this);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    //toolBar->setIconSize(QSize(14,14));
    //toolBar->layout()->setMargin(0);

    Nodes *nodes = fact->findParent<Nodes *>();
    if (nodes) {
        aUpload = new QAction(MaterialIcon("upload"), tr("Upload"), this);
        connect(aUpload, &QAction::triggered, nodes, [nodes]() { nodes->f_upload->trigger(); });
        connect(nodes->f_upload, &Fact::enabledChanged, this, [=]() {
            Nodes *nodes = fact->findParent<Nodes *>();
            aUpload->setEnabled(nodes && nodes->f_upload->enabled());
        });
        aUpload->setEnabled(nodes->f_upload->enabled());
        toolBar->addAction(aUpload);
        toolBar->widgetForAction(aUpload)->setObjectName("greenAction");
    }

    NodeItem *node = fact->findParent<NodeItem *>();
    if (node) {
        setWindowTitle(
            QString("%1-%2: %3").arg(node->title()).arg(node->status()).arg(windowTitle()));
    }

    aUndo = new QAction(MaterialIcon("undo"), tr("Revert"), this);
    connect(aUndo, &QAction::triggered, fact, &Fact::restore);
    connect(fact, &Fact::modifiedChanged, this, [=]() { aUndo->setEnabled(fact->modified()); });
    aUndo->setEnabled(fact->modified());
    toolBar->addAction(aUndo);

    toolBar->addSeparator();

    aSep = toolBar->addSeparator();

    //close button
    QAction *aClose = new QAction(MaterialIcon("close"), tr("Close"), this);
    connect(aClose, &QAction::triggered, this, [=]() {
        if (aboutToClose())
            accept();
    });
    toolBar->addAction(aClose);
    toolBar->widgetForAction(aClose)->setObjectName("redAction");

    setLayout(vlayout);
    vlayout->addWidget(toolBar);

    doRestoreGeometry();
    connect(this, &FactDelegateDialog::finished, this, &FactDelegateDialog::doSaveGeometry);
    connect(this, &FactDelegateDialog::finished, this, &FactDelegateDialog::deleteLater);
    connect(fact, &Fact::removed, this, [this]() { delete this; });
    connect(fact, &Fact::destroyed, this, &FactDelegateDialog::deleteLater);
}
FactDelegateDialog::~FactDelegateDialog()
{
    //qDebug()<<"delete FactDelegateDialog";
    dlgMap.remove(dlgMap.key(this));
}
//=============================================================================
void FactDelegateDialog::addAction(QAction *a)
{
    toolBar->insertAction(aSep, a);
}
//=============================================================================
void FactDelegateDialog::setWidget(QWidget *w)
{
    QSizePolicy sp = w->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    w->setSizePolicy(sp);
    //vlayout->insertWidget(0,w);
    vlayout->addWidget(w);
    widget = w;

    QString s = fact->titlePath();
    if (dlgMap.contains(fact)) {
        FactDelegateDialog *dlg = dlgMap.value(fact);
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
        deleteLater();
        return;
    }
    dlgMap.insert(fact, this);

    doRestoreGeometry();
    show();
}
//=============================================================================
void FactDelegateDialog::closeEvent(QCloseEvent *event)
{
    doSaveGeometry();
    if (!aboutToClose()) {
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}
//=============================================================================
void FactDelegateDialog::doSaveGeometry()
{
    QSettings s;
    s.beginGroup("geometry");
    s.setValue(objectName() + "_Geometry", saveGeometry());
    if (widget) {
        foreach (QSplitter *w, widget->findChildren<QSplitter *>()) {
            s.setValue(objectName() + "_" + w->objectName() + "State", w->saveState());
        }
    }
}
//=============================================================================
void FactDelegateDialog::doRestoreGeometry()
{
    QSettings s;
    s.beginGroup("geometry");
    restoreGeometry(s.value(objectName() + "_Geometry").toByteArray());
    if (widget) {
        foreach (QSplitter *w, widget->findChildren<QSplitter *>()) {
            w->restoreState(s.value(objectName() + "_" + w->objectName() + "State").toByteArray());
        }
    }
}
//=============================================================================
