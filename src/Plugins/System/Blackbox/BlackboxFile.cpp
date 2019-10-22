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
#include "BlackboxFile.h"
#include "BlackboxReader.h"

#include <App/AppLog.h>

#include <QFileDialog>
//=============================================================================
BlackboxFile::BlackboxFile(Fact *parent)
    : BlackboxItem(parent, "imp", tr("Import"), tr("Import binary data from file"), Group)
{
    setIcon("import");
    f_stats->setIcon("folder-open");
    f_callsign->setValue("FILE");
}
//=============================================================================
void BlackboxFile::updateStats()
{
    totalSize = fi.size();
    BlackboxItem::updateStats();
    f_stats->setDescr(fi.fileName());
    updateActions();
}
//=============================================================================
void BlackboxFile::readNext()
{
    if (!file.isOpen())
        return;
    reader->processData(file.read(1024));

    if (file.atEnd()) {
        apxMsg() << tr("Blackbox import finished");
        stop();
        return;
    }
    qint64 p = file.pos() * 100 / file.size();
    setProgress(p);

    QTimer::singleShot(1, this, &BlackboxFile::readNext);
}
//=============================================================================
void BlackboxFile::getStats()
{
    fi = QFileInfo();
    updateStats();

    QDir defaultDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    QDir dir = QDir(QSettings()
                        .value(QString("SharePath_%1").arg(name()), defaultDir.canonicalPath())
                        .toString());
    if (!dir.exists())
        dir = defaultDir;
    QFileDialog dlg(nullptr, descr(), dir.canonicalPath());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setViewMode(QFileDialog::Detail);
    if (!(dlg.exec() && dlg.selectedFiles().size() == 1)) {
        return;
    }

    QSettings().setValue(QString("SharePath_%1").arg(name()), dlg.directory().absolutePath());

    QString fname = dlg.selectedFiles().first();
    fi = QFileInfo(fname);
    updateStats();
}
void BlackboxFile::download()
{
    if (!fi.exists())
        return;
    if (fi.size() <= 0)
        return;
    BlackboxItem::download();
    file.setFileName(fi.filePath());
    if (!file.open(QFile::ReadOnly)) {
        apxMsgW() << tr("Can't read file") << file.fileName();
        stop();
        return;
    }
    readNext();
}
void BlackboxFile::stop()
{
    BlackboxItem::stop();
    file.close();
}
//=============================================================================
