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
#include "FactDelegateScript.h"
#include "SourceEdit.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <ApxMisc/MaterialIcon.h>
#include <Pawn/PawnCompiler.h>
#include <Vehicles/Vehicles.h>
#include <QtWidgets>
//=============================================================================
FactDelegateScript::FactDelegateScript(Fact *fact, QWidget *parent)
    : FactDelegateDialog(fact, parent)
{
    aCompile = new QAction(MaterialIcon("code-tags-check"), tr("Compile"), this);
    addAction(aCompile);

    aLoad = new QAction(MaterialIcon("folder-open"), tr("Load"), this);
    connect(aLoad, &QAction::triggered, this, &FactDelegateScript::aLoad_triggered);
    addAction(aLoad);

    aSave = new QAction(MaterialIcon("content-save"), tr("Save"), this);
    connect(aSave, &QAction::triggered, this, &FactDelegateScript::aSave_triggered);
    addAction(aSave);

    QWidget *w = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(w);
    w->setLayout(layout);
    layout->setMargin(0);
    layout->setSpacing(0);

    QSplitter *splitter = new QSplitter(Qt::Vertical, w);
    splitter->setChildrenCollapsible(false);
    layout->addWidget(splitter);
    //layout->setMargin(0);
    //layout->setSpacing(0);

    editor = new SourceEdit(w);
    Vehicle *vehicle = fact->findParent<Vehicle *>();
    if (vehicle) {
        QStringList st;
        foreach (QString s, vehicle->f_mandala->names)
            st.append("f_" + s);
        st.append(vehicle->f_mandala->constants.keys());
        editor->addKeywords(st);
    }
    splitter->addWidget(editor);

    logList = new QListWidget(w);
    connect(logList, &QListWidget::itemClicked, this, &FactDelegateScript::logView_itemClicked);
    splitter->addWidget(logList);

    setWidget(w);

    pawncc = new PawnCompiler(fact);
    connect(pawncc, &PawnCompiler::compiled, this, &FactDelegateScript::updateLog);
    pawncc->compile();

    editor->setPlainText(fact->value().toString());
    connect(fact, &Fact::valueChanged, this, [=]() {
        pawncc->compile();
        QString s = fact->value().toString();
        if (editor->toPlainText() != s) {
            editor->setPlainText(s);
        }
    });

    connect(aCompile, &QAction::triggered, this, [=]() {
        editor->cleanText();
        QString s = editor->toPlainText();
        fact->setValue(s.simplified().isEmpty() ? QString() : s);
    });
}
//=============================================================================
bool FactDelegateScript::aboutToUpload(void)
{
    aCompile->trigger();
    return true;
}
//=============================================================================
bool FactDelegateScript::aboutToClose(void)
{
    aCompile->trigger();
    return true;
}
//=============================================================================
void FactDelegateScript::aSave_triggered(void)
{
    QFileDialog dlg(this, aSave->toolTip(), AppDirs::scripts().canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    if (!scrName.isEmpty())
        dlg.selectFile(AppDirs::scripts().filePath(scrName + ".p"));
    QStringList filters;
    filters << tr("Script files") + " (*.p)" << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);
    if (!(dlg.exec() && dlg.selectedFiles().size() == 1))
        return;
    saveToFile(dlg.selectedFiles().first());
}
//=============================================================================
void FactDelegateScript::aLoad_triggered(void)
{
    QFileDialog dlg(this, aLoad->toolTip(), AppDirs::scripts().canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    if (!scrName.isEmpty())
        dlg.selectFile(AppDirs::scripts().filePath(scrName + ".p"));
    QStringList filters;
    filters << tr("Script files") + " (*.p)" << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);
    if (!(dlg.exec() && dlg.selectedFiles().size() == 1))
        return;
    loadFromFile(dlg.selectedFiles().first());
}
//=============================================================================
void FactDelegateScript::updateLog()
{
    logList->clear();
    //label->setText(field->data(NodesItem::tc_value,Qt::DisplayRole).toString());
    uint icnt = 0;
    foreach (QString s, pawncc->getLog().split("\n", QString::SkipEmptyParts)) {
        if (s.startsWith("Pawn"))
            continue;
        if (s.contains("error") || s.contains("warning")) {
            //QStringList w=s.split(':');
            QListWidgetItem *i = new QListWidgetItem(s, logList);
            i->setBackground(s.contains("warning") ? QColor(220, 220, 200) : QColor(220, 150, 150));
            i->setForeground(Qt::black);
            icnt++;
        } else {
            new QListWidgetItem(s, logList);
            icnt++;
        }
    }
    if (!icnt) {
        if (!pawncc->outData().isEmpty())
            new QListWidgetItem(tr("Success"), logList);
    }
    if (pawncc->outData().isEmpty())
        new QListWidgetItem(tr("Empty script"), logList);
}
//=============================================================================
bool FactDelegateScript::saveToFile(QString fname)
{
    QFile file(fname);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot write file")
                  << QString("%1:\n%2.").arg(fname).arg(file.errorString());
        return false;
    }
    scrName = QFileInfo(fname).baseName();
    //label->setText(scrName);
    editor->cleanText();
    QTextStream s(&file);
    s << editor->toPlainText();
    return true;
}
//=============================================================================
bool FactDelegateScript::loadFromFile(QString fname)
{
    QFile file(fname);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot read file")
                  << QString("%1:\n%2.").arg(fname).arg(file.errorString());
        return false;
    }
    scrName = QFileInfo(fname).baseName();
    //label->setText(scrName);
    QTextStream s(&file);
    fact->setValue(s.readAll());
    return true;
}
//=============================================================================
void FactDelegateScript::logView_itemClicked(QListWidgetItem *item)
{
    QString s = item->text();
    QRegExp exp("\\((.*)\\)");
    if (exp.indexIn(s.left(s.indexOf(':'))) >= 0) {
        bool ok;
        int line = exp.cap(1).toInt(&ok);
        if ((!ok) && exp.cap(1).contains("--"))
            line = exp.cap(1).left(exp.cap(1).indexOf("--")).toInt(&ok);
        if (ok && line >= 0) {
            QTextCursor text_cursor(editor->document()->findBlockByLineNumber(line - 1));
            text_cursor.select(QTextCursor::LineUnderCursor);
            editor->setTextCursor(text_cursor);
        }
    }
}
//=============================================================================
