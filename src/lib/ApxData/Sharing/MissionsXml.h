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
#ifndef MissionsXml_H
#define MissionsXml_H
//=============================================================================
#include "ShareXml.h"
#include <Database/MissionsDB.h>
#include <Dictionary/DictMission.h>
//=============================================================================
class MissionsXmlExport : public ShareXmlExport
{
    Q_OBJECT
public:
    explicit MissionsXmlExport(QString hash, QString title, QString fileName);

protected:
    bool run(QSqlQuery &query);
    bool write(QDomNode &dom);

private:
    DBReqMissionsLoad req;
    QVariantMap details;
    void write(QDomNode &dom,
               const QString &sectionName,
               const QString &elementName,
               const QList<DictMission::Item> &items);
};
//=============================================================================
class MissionsXmlImport : public ShareXmlImport
{
    Q_OBJECT
public:
    explicit MissionsXmlImport(QString title, QString fileName);

protected:
    bool read(const QDomNode &dom);
    bool readOldFormat(const QDomNode &dom, int fmt);
    bool save(QSqlQuery &query);

private:
    DictMission::Mission mission;
    QVariantMap details;

    int read(const QDomNode &dom,
             const QString &sectionName,
             const QString &elementName,
             QList<DictMission::Item> &items);
    int readOldFormat(const QDomNode &dom,
                      const QString &sectionName,
                      const QString &elementName,
                      QList<DictMission::Item> &items);
};
//=============================================================================
#endif
