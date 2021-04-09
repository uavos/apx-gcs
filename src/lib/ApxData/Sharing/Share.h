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
                   QString type,
                   QString dataTitle,
                   QDir defaultDir,
                   FactBase::Flags flags = FactBase::Flags(Group));

    Fact *f_export;
    Fact *f_import;

    QByteArray loadData(QString fileName);
    bool saveData(QByteArray data, QString fileName);

protected:
    QString _dataTitle;
    QDir _defaultDir;
    QDir _templatesDir;

    QStringList _exportFormats;
    QStringList _importFormats;

    virtual QString getDefaultTitle() { return QString(); }
    virtual bool exportRequest(QString format, QString fileName) { return false; }
    virtual bool importRequest(QString format, QString fileName) { return false; }

protected slots:
    void exportTriggered();
    void importTriggered();

    void _exported(QString fileName);
    void _imported(QString fileName, QString title = QString());

    virtual void syncTemplates() {}

signals:
    void imported(QString fileName, QString title);
    void exported(QString fileName);
};
