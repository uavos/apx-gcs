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
#ifndef Share_H
#define Share_H
//=============================================================================
#include "ShareXml.h"
#include <Fact/Fact.h>
#include <QDomDocument>
#include <QtCore>
//=============================================================================
class Share : public Fact
{
    Q_OBJECT

public:
    explicit Share(Fact *parent,
                   QString dataTitle,
                   QString fileType,
                   QDir defaultDir,
                   //additional export formats
                   const QStringList &exportFileTypes = QStringList(),
                   FactBase::Flags flags = FactBase::Flags(Group));

    Fact *f_export;
    Fact *f_import;

protected:
    QString dataTitle;
    QString fileType;
    QDir defaultDir;
    QStringList exportFileTypes;

    virtual QString defaultExportFileName() const;
    virtual ShareXmlExport *exportRequest(QString title, QString fileName);

    virtual QString defaultImportFileName() const;
    virtual ShareXmlImport *importRequest(QString title, QString fileName);

private slots:
    void syncTemplates();

    void dbExported(QByteArray data, QString fileName);
    void dbImported(QString hash, QString title);

protected slots:
    void exportTriggered();
    void importTriggered();

signals:
    void imported(QString hash, QString title);
    void exported(QByteArray data, QString fileName);
};
//=============================================================================
#endif
