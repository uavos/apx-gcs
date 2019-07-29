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
#ifndef SiteEdit_H
#define SiteEdit_H
//=============================================================================
#include <QtCore>
#include <Fact/Fact.h>
#include <QtLocation>
#include <Mission/LookupMissions.h>
class LookupMissions;
//=============================================================================
class SiteEdit : public Fact
{
    Q_OBJECT

public:
    explicit SiteEdit(Fact *parent,
                      const QString &name,
                      const QString &title,
                      const QString &descr,
                      QVariantMap modelData);

    Fact *f_title;
    Fact *f_descr;
    Fact *f_radius;

    Fact *f_latitude;
    Fact *f_longitude;

    FactAction *a_add;
    FactAction *a_remove;
    FactAction *a_missions;

    LookupMissions *f_missions;

    QVariantMap modelData;
    void setModelData(QVariantMap v);

private:
    bool blockUpdateItemData;

private slots:
    void saveToModelData();
    void lookupMissions();

public slots:
    void reset();
    void loadFromModelData();
    void updateFromEditedModelData(int i, QVariantMap v);

signals:
    void addTriggered(QVariantMap item);
    void removeTriggered(QVariantMap item);

    void siteEdited(QVariantMap item);
};
//=============================================================================
#endif
