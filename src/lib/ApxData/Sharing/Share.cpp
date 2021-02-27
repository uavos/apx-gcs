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
#include "Share.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <App/AppSettings.h>
#include <QFileDialog>

#include "ShareExport.h"
#include "ShareImport.h"

Share::Share(Fact *parent, QString dataTitle, QDir defaultDir, Flags flags)
    : Fact(parent,
           "share",
           tr("Share"),
           tr("Share").append(" ").append(dataTitle),
           flags,
           "share-variant")
    , dataTitle(dataTitle)
    , defaultDir(defaultDir)
{
    f_export = new Fact(this,
                        "exp",
                        tr("Export").append(" ").append(dataTitle),
                        tr("Save data to file"),
                        CloseOnTrigger,
                        "export");
    connect(f_export, &Fact::triggered, this, &Share::exportTriggered, Qt::QueuedConnection);

    f_import = new Fact(this,
                        "imp",
                        tr("Import").append(" ").append(dataTitle),
                        tr("Load data from file"),
                        CloseOnTrigger,
                        "import");
    connect(f_import, &Fact::triggered, this, &Share::importTriggered, Qt::QueuedConnection);

    // connect(App::instance(), &App::loadingFinished, this, [this]() {
    //     QTimer::singleShot(1000, this, &Share::_syncTemplates);
    // });
}

void Share::add(ShareExport *format)
{
    _exportFormats.append(format);
    connect(format, &ShareExport::exported, this, &Share::_exported, Qt::QueuedConnection);
}
void Share::add(ShareImport *format)
{
    _importFormats.append(format);
    connect(format, &ShareImport::imported, this, &Share::_imported, Qt::QueuedConnection);
}

void Share::exportTriggered()
{
    if (_exportFormats.isEmpty())
        return;
    auto fmt = _exportFormats.first();

    if (!defaultDir.exists())
        defaultDir.mkpath(".");

    QFileDialog dlg(nullptr, f_export->descr(), defaultDir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    dlg.setViewMode(QFileDialog::Detail);
    QStringList filters;
    filters << dataTitle + " " + tr("files") + " (*." + fmt->type() + ")";
    for (auto i : _exportFormats.mid(1)) {
        filters << i->type().toUpper() + " " + tr("format") + " (*." + i->type() + ")";
    }
    dlg.setDefaultSuffix(fmt->type());

    QString ftitle = getDefaultTitle();
    if (!ftitle.isEmpty()) {
        dlg.selectFile(defaultDir.filePath(ftitle + "." + fmt->type()));
    }
    dlg.setNameFilters(filters);

    if (!(dlg.exec() && dlg.selectedFiles().size() == 1))
        return;

    QFileInfo fi(dlg.selectedFiles().first());

    fmt = nullptr;
    for (auto i : _exportFormats) {
        if (i->type() != fi.suffix())
            continue;
        fmt = i;
        break;
    }
    if (!fmt || !exportRequest(fmt, fi.filePath())) {
        apxMsg() << tr("Can't export").append(':') << fi.fileName();
        return;
    }
    emit menuBack();
}
void Share::_exported(QString fileName)
{
    apxMsg() << QString("%1 %2:").arg(dataTitle).arg(tr("exported"))
             << QFileInfo(fileName).fileName();
    emit exported(fileName);
}

void Share::importTriggered()
{
    if (_importFormats.isEmpty())
        return;
    auto fmt = _importFormats.first();

    if (!defaultDir.exists())
        defaultDir.mkpath(".");

    QDir dir = QDir(QSettings()
                        .value(QString("SharePath_%1").arg(fmt->type()), defaultDir.canonicalPath())
                        .toString());
    if (!dir.exists())
        dir = defaultDir;

    QFileDialog dlg(nullptr, f_import->descr(), dir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setViewMode(QFileDialog::Detail);
    QStringList filters;
    filters << dataTitle + " " + tr("files") + " (*." + fmt->type() + ")";
    for (auto i : _importFormats.mid(1)) {
        filters << i->type().toUpper() + " " + tr("format") + " (*." + i->type() + ")";
    }
    dlg.setDefaultSuffix(fmt->type());

    QString ftitle = getDefaultTitle();
    if (!ftitle.isEmpty()) {
        dlg.selectFile(defaultDir.filePath(ftitle + "." + fmt->type()));
    }
    filters << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);

    if (!(dlg.exec() && dlg.selectedFiles().size() >= 1))
        return;
    QSettings().setValue(QString("SharePath_%1").arg(fmt->type()), dlg.directory().absolutePath());
    emit menuBack();

    for (auto fileName : dlg.selectedFiles()) {
        QFileInfo fi(fileName);
        apxMsg() << tr("Importing")
                 << QString("%1: %2...").arg(dataTitle).arg(fi.completeBaseName());

        fmt = nullptr;
        for (auto i : _importFormats) {
            if (i->type() != fi.suffix())
                continue;
            fmt = i;
            break;
        }
        if (!fmt || !importRequest(fmt, fi.filePath())) {
            apxMsg() << tr("Can't import").append(':') << fi.fileName();
            continue;
        }
    }
}
void Share::_imported(QString fileName, QString hash, QString title)
{
    Q_UNUSED(hash)
    apxMsg() << QString("%1 %2:").arg(dataTitle).arg(tr("imported"))
             << (title.isEmpty() ? QFileInfo(fileName).completeBaseName() : title);
    emit imported(fileName, hash, title);
}
