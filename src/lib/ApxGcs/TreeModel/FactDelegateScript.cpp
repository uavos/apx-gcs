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
#include "FactDelegateScript.h"
#include "SourceEdit.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <ApxMisc/MaterialIcon.h>
#include <Nodes/NodeScript.h>
#include <Vehicles/Vehicles.h>
#include <QtWidgets>

FactDelegateScript::FactDelegateScript(Fact *fact, QWidget *parent)
    : FactDelegateDialog(fact, parent)
{
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    aCompile = new QAction(MaterialIcon("code-tags-check"), tr("Compile"), this);
    addAction(aCompile);

    aLoad = new QAction(MaterialIcon("folder-open"), tr("Load"), this);
    connect(aLoad, &QAction::triggered, this, &FactDelegateScript::aLoad_triggered);
    addAction(aLoad);

    aSave = new QAction(MaterialIcon("content-save"), tr("Save"), this);
    connect(aSave, &QAction::triggered, this, &FactDelegateScript::aSave_triggered);
    addAction(aSave);

    eTitle = new QLineEdit(this);
    eTitle->setPlaceholderText(tr("script title"));
    toolBar->addWidget(eTitle);

    QWidget *w = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(w);
    w->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QSplitter *splitter = new QSplitter(Qt::Vertical, w);
    splitter->setChildrenCollapsible(false);
    layout->addWidget(splitter);
    //layout->setMargin(0);
    //layout->setSpacing(0);

    editor = new SourceEdit(w);
    splitter->addWidget(editor);

    logList = new QListWidget(w);
    logList->setFont(App::getMonospaceFont());
    connect(logList, &QListWidget::itemClicked, this, &FactDelegateScript::logView_itemClicked);
    splitter->addWidget(logList);

    setWidget(w);

    nodeScript = qobject_cast<NodeScript *>(fact->property("script").value<QObject *>());
    connect(nodeScript, &NodeScript::compiled, this, &FactDelegateScript::updateLog);
    updateLog();

    editor->addKeywords(nodeScript->constants.keys());

    connect(fact,
            &Fact::valueChanged,
            this,
            &FactDelegateScript::updateEditorText,
            Qt::QueuedConnection);
    updateEditorText();

    connect(aCompile, &QAction::triggered, this, &FactDelegateScript::updateFactValue);

    connect(eTitle, &QLineEdit::editingFinished, aCompile, &QAction::trigger);

    // open vscode
    auto cc_plugin = App::plugin("ScriptCompiler");
    if (cc_plugin) {
        auto c = cc_plugin->control;
        auto use_vscode = c->property("use_vscode").value<Fact *>()->value().toBool();
        if (use_vscode) {
            launch_vscode();
        }
    }
}
void FactDelegateScript::updateEditorText()
{
    QString s = nodeScript->source();
    if (editor->toPlainText() != s) {
        editor->setPlainText(s);
    }
    s = nodeScript->title();
    if (eTitle->text() != s) {
        eTitle->setText(s);
    }
}
void FactDelegateScript::updateFactValue()
{
    editor->cleanText();
    QString s = editor->toPlainText();
    if (s.simplified().isEmpty())
        s.clear();
    nodeScript->setSource(eTitle->text(), s);
}

bool FactDelegateScript::aboutToUpload(void)
{
    aCompile->trigger();
    return true;
}

bool FactDelegateScript::aboutToClose(void)
{
    aCompile->trigger();
    return true;
}

void FactDelegateScript::aSave_triggered(void)
{
    aCompile->trigger();
    QFileDialog dlg(this, aSave->toolTip(), AppDirs::scripts().canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    if (!eTitle->text().isEmpty())
        dlg.selectFile(AppDirs::scripts().filePath(eTitle->text() + ".cpp"));
    QStringList filters;
    filters << tr("Source files") + " (*.cpp)" << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);
    if (!(dlg.exec() && dlg.selectedFiles().size() == 1))
        return;
    QString fname = dlg.selectedFiles().first();
    nodeScript->saveToFile(fname);
}

void FactDelegateScript::aLoad_triggered(void)
{
    QFileDialog dlg(this, aLoad->toolTip(), AppDirs::scripts().canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    if (!eTitle->text().isEmpty())
        dlg.selectFile(AppDirs::scripts().filePath(eTitle->text() + ".cpp"));
    QStringList filters;
    filters << tr("Source files") + " (*.cpp)" << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);
    if (!(dlg.exec() && dlg.selectedFiles().size() == 1))
        return;
    QString fname = dlg.selectedFiles().first();
    nodeScript->loadFromFile(fname);
}

void FactDelegateScript::updateLog()
{
    logList->clear();
    //label->setText(field->data(NodesItem::tc_value,Qt::DisplayRole).toString());
    uint icnt = 0;
    for (auto s : nodeScript->log().split("\n", Qt::SkipEmptyParts)) {
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
        if (!nodeScript->code().isEmpty())
            new QListWidgetItem(tr("Success"), logList);
    }
    if (nodeScript->code().isEmpty())
        new QListWidgetItem(tr("Empty script"), logList);
}

void FactDelegateScript::logView_itemClicked(QListWidgetItem *item)
{
    QString s = item->text();
    static QRegularExpression exp("\\((.*)\\)");
    auto match = exp.match(s.left(s.indexOf(':')));
    if (match.hasMatch()) {
        auto c = match.captured(1);
        bool ok;
        int line = c.toInt(&ok);
        if ((!ok) && c.contains("--"))
            line = c.left(match.captured(1).indexOf("--")).toInt(&ok);
        if (ok && line >= 0) {
            QTextCursor text_cursor(editor->document()->findBlockByLineNumber(line - 1));
            text_cursor.select(QTextCursor::LineUnderCursor);
            editor->setTextCursor(text_cursor);
        }
    }
}

void FactDelegateScript::launch_vscode()
{
    auto exec = QStandardPaths::findExecutable("code");

    if (exec.isEmpty()) {
        // try to find other paths
        const QStringList paths{"/usr/local/bin", "/usr/bin"};
        exec = QStandardPaths::findExecutable("code", paths);
    }

    if (exec.isEmpty()) {
        qWarning() << "vscode not found";
        return;
    }
    auto title = eTitle->text();
    if (title.isEmpty()) {
        apxMsgW() << tr("Script title is empty");
        return;
    }

    QDir workspace = AppDirs::scripts();
    QDir src = QDir(AppDirs::scripts().absoluteFilePath("src"));
    if (!src.exists())
        src.mkpath(".");

    QString fname = src.absoluteFilePath(title + ".cpp");
    nodeScript->saveToFile(fname);

    QStringList args;
    args << workspace.absolutePath();
    args << fname;
    QProcess::startDetached(exec, args);
}
