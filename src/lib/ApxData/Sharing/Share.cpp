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
#include "Share.h"

#include <ApxLog.h>
#include <QFileDialog>
//=============================================================================
Share::Share(Fact *parent,
             QString dataTitle,
             QString fileType,
             QDir defaultDir,
             const QStringList &exportFileTypes)
    : Fact(parent, "share", tr("Share"), tr("Share").append(" ").append(dataTitle))
    , dataTitle(dataTitle)
    , fileType(fileType)
    , defaultDir(defaultDir)
    , exportFileTypes(exportFileTypes)
{
    setIcon("share-variant");

    f_export = new Fact(this, "exp", tr("Export"), tr("Export").append(" ").append(dataTitle));
    f_export->setIcon("export");
    connect(f_export, &Fact::triggered, this, &Fact::actionTriggered); //to close popups
    connect(f_export, &Fact::triggered, this, &Share::exportTriggered);

    f_import = new Fact(this, "imp", tr("Import"), tr("Import").append(" ").append(dataTitle));
    f_import->setIcon("import");
    connect(f_import, &Fact::triggered, this, &Fact::actionTriggered);
    connect(f_import, &Fact::triggered, this, &Share::importTriggered);
}
//=============================================================================
QString Share::defaultExportFileName() const
{
    return QString();
}
QString Share::defaultImportFileName() const
{
    return defaultExportFileName();
}
ShareXmlExport *Share::exportRequest(QString title, QString fileName)
{
    qDebug() << "not implemented" << title << fileName;
    return nullptr;
}
ShareXmlImport *Share::importRequest(QString title, QString fileName)
{
    Q_UNUSED(fileName)
    qDebug() << "not implemented" << title;
    return nullptr;
}
//=============================================================================
void Share::exportTriggered()
{
    if (!defaultDir.exists())
        defaultDir.mkpath(".");
    QFileDialog dlg(nullptr, f_export->descr(), defaultDir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    dlg.setViewMode(QFileDialog::Detail);
    QStringList filters;
    filters << dataTitle + " " + tr("files") + " (*." + fileType + ")";
    foreach (QString exType, exportFileTypes) {
        filters << exType.toUpper() + " " + tr("format") + " (*." + exType + ")";
    }
    filters << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);
    dlg.setDefaultSuffix(fileType);
    QString fname = defaultExportFileName();
    if (!fname.isEmpty()) {
        dlg.selectFile(defaultDir.filePath(fname + "." + fileType));
    }
    /*connect(&dlg, &QFileDialog::filterSelected, this, [](const QString &filter) {
        apxMsg() << filter;
    });*/
    if (!(dlg.exec() && dlg.selectedFiles().size() == 1))
        return;
    emit menuBack();
    fname = dlg.selectedFiles().first();
    ShareXmlExport *req = exportRequest(QFileInfo(fname).baseName(), fname);
    if (!req)
        return;
    connect(req, &ShareXmlExport::exported, this, &Share::exported, Qt::QueuedConnection);
    req->exec();
}
void Share::exported(QByteArray data, QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot write file").append(":") << fileName
                  << QString("(%1)").arg(file.errorString());
        return;
    }
    file.write(data);
    file.close();
    apxMsg() << QString("%1 %2:").arg(dataTitle).arg(tr("exported"))
             << QFileInfo(fileName).fileName();
}
//=============================================================================
void Share::importTriggered()
{
    if (!defaultDir.exists())
        defaultDir.mkpath(".");
    QDir dir = QDir(QSettings()
                        .value(QString("SharePath_%1").arg(fileType), defaultDir.canonicalPath())
                        .toString());
    if (!dir.exists())
        dir = defaultDir;
    QFileDialog dlg(nullptr, f_import->descr(), dir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setViewMode(QFileDialog::Detail);
    QStringList filters;
    filters << dataTitle + " " + tr("files") + " (*." + fileType + ")" << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);
    dlg.setDefaultSuffix(fileType);
    QString ftitle = defaultImportFileName();
    if (!ftitle.isEmpty()) {
        dlg.selectFile(ftitle + "." + fileType);
    }
    if (!(dlg.exec() && dlg.selectedFiles().size() >= 1))
        return;
    emit menuBack();
    QSettings().setValue(QString("SharePath_%1").arg(fileType), dlg.directory().absolutePath());

    foreach (QString fname, dlg.selectedFiles()) {
        QFileInfo fi(fname);
        ShareXmlImport *req = importRequest(fi.completeBaseName(), fname);
        if (!req)
            continue;
        apxMsg() << tr("Importing")
                 << QString("%1: %2...").arg(dataTitle).arg(fi.completeBaseName());
        connect(req, &ShareXmlImport::imported, this, &Share::imported, Qt::QueuedConnection);
        req->exec();
    }
}
void Share::imported(QString hash, QString title)
{
    Q_UNUSED(hash)
    apxMsg() << QString("%1 %2:").arg(dataTitle).arg(tr("imported")) << title;
}
//=============================================================================
//=============================================================================
