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
#pragma once

#include <Fact/Fact.h>
#include <QtCore>

class ShareExport;
class ShareImport;
class Share : public Fact
{
    Q_OBJECT

public:
    explicit Share(Fact *parent,
                   QString dataTitle,
                   QDir defaultDir,
                   FactBase::Flags flags = FactBase::Flags(Group));

    Fact *f_export;
    Fact *f_import;

protected:
    QString dataTitle;
    QDir defaultDir;

    QList<ShareExport *> _exportFormats;
    QList<ShareImport *> _importFormats;

    virtual QString getDefaultTitle() { return QString(); }
    virtual bool exportRequest(ShareExport *format, QString fileName) { return false; }
    virtual bool importRequest(ShareImport *format, QString fileName) { return false; }

    void add(ShareExport *format);
    void add(ShareImport *format);

private slots:
    void _exported(QString fileName);
    void _imported(QString fileName, QString hash, QString title);

protected slots:
    void exportTriggered();
    void importTriggered();

signals:
    void imported(QString fileName, QString hash, QString title);
    void exported(QString fileName);
};
