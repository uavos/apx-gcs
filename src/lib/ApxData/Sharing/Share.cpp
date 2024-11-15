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

Share::Share(Fact *parent, QString type, QString dataTitle, QDir defaultDir, Flags flags)
    : Fact(parent,
           "share",
           tr("Share"),
           tr("Share").append(" ").append(dataTitle),
           flags,
           "share-variant")
    , _dataTitle(dataTitle)
    , _defaultDir(defaultDir)
{
    _exportFormats << type;
    _importFormats << type;

    f_export = new Fact(this,
                        "exp",
                        tr("Export").append(" ").append(_dataTitle),
                        tr("Save data to file"),
                        CloseOnTrigger,
                        "export");
    connect(f_export, &Fact::triggered, this, &Share::exportTriggered, Qt::QueuedConnection);

    f_import = new Fact(this,
                        "imp",
                        tr("Import").append(" ").append(_dataTitle),
                        tr("Load data from file"),
                        CloseOnTrigger,
                        "import");
    connect(f_import, &Fact::triggered, this, &Share::importTriggered, Qt::QueuedConnection);

    _templatesDir = QDir(AppDirs::res().absoluteFilePath("templates/share"),
                         QString("*.%1").arg(_importFormats.first()));
    connect(App::instance(), &App::loadingFinished, this, [this]() {
        QTimer::singleShot(1000, this, &Share::syncTemplates);
    });
}

void Share::exportTriggered()
{
    if (_exportFormats.isEmpty())
        return;
    auto fmt = _exportFormats.first();

    if (!_defaultDir.exists())
        _defaultDir.mkpath(".");

    QFileDialog dlg(nullptr, f_export->descr(), _defaultDir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    dlg.setViewMode(QFileDialog::Detail);
    QStringList filters;
    filters << _dataTitle + " " + tr("files") + " (*." + fmt + ")";
    for (auto i : _exportFormats.mid(1)) {
        filters << i.toUpper() + " " + tr("format") + " (*." + i + ")";
    }
    dlg.setDefaultSuffix(fmt);

    QString ftitle = getDefaultTitle();
    if (!ftitle.isEmpty()) {
        dlg.selectFile(_defaultDir.filePath(ftitle + "." + fmt));
    }
    dlg.setNameFilters(filters);

    if (!(dlg.exec() && dlg.selectedFiles().size() == 1))
        return;

    QFileInfo fi(dlg.selectedFiles().first());

    _defaultDir.setPath(dlg.directory().absolutePath());

    if (!exportRequest(fi.suffix(), fi.absoluteFilePath())) {
        apxMsg() << tr("Can't export").append(':') << fi.fileName();
        return;
    }

    emit menuBack();
}
void Share::_exported(QString fileName)
{
    apxMsg() << QString("%1 %2:").arg(_dataTitle).arg(tr("exported"))
             << QFileInfo(fileName).fileName();
    emit exported(fileName);
}

void Share::importTriggered()
{
    if (_importFormats.isEmpty())
        return;
    auto fmt = _importFormats.first();

    if (!_defaultDir.exists())
        _defaultDir.mkpath(".");

    QDir dir = QDir(
        QSettings().value(QString("SharePath_%1").arg(fmt), _defaultDir.canonicalPath()).toString());
    if (!dir.exists())
        dir = _defaultDir;

    QFileDialog dlg(nullptr, f_import->descr(), dir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setViewMode(QFileDialog::Detail);
    QStringList filters;
    filters << _dataTitle + " " + tr("files") + " (*." + fmt + ")";
    for (auto i : _importFormats.mid(1)) {
        filters << i.toUpper() + " " + tr("format") + " (*." + i + ")";
    }
    dlg.setDefaultSuffix(fmt);

    QString ftitle = getDefaultTitle();
    if (!ftitle.isEmpty()) {
        dlg.selectFile(_defaultDir.filePath(ftitle + "." + fmt));
    }
    filters << tr("Any files") + " (*)";
    dlg.setNameFilters(filters);

    if (!(dlg.exec() && dlg.selectedFiles().size() >= 1))
        return;
    QSettings().setValue(QString("SharePath_%1").arg(fmt), dlg.directory().absolutePath());
    emit menuBack();

    if (!importRequest(dlg.selectedFiles())) {
        apxMsgW() << tr("%1 import error").arg(_dataTitle);
    }
}
void Share::_imported(QString fileName, QString title)
{
    if (title.isEmpty())
        title = QFileInfo(fileName).completeBaseName();

    apxMsg() << QString("%1 %2:").arg(_dataTitle).arg(tr("imported")) << title;
    emit imported(fileName, title);
}

QByteArray Share::loadData(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot read file").append(":") << fileName
                  << QString("(%1)").arg(file.errorString());
        return QByteArray();
    }
    return file.readAll();
}

bool Share::saveData(QByteArray data, QString fileName)
{
    qDebug() << fileName << data.size();

    if (data.isEmpty())
        return false;
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot write file").append(":") << fileName
                  << QString("(%1)").arg(file.errorString());
        return false;
    }
    file.write(data);
    file.close();
    return true;
}
