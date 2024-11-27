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
#include <QtLocation>
class LookupMissions;

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

    Fact *a_add;
    Fact *a_remove;

    QVariantMap modelData;
    void setModelData(QVariantMap v);

private:
    bool blockUpdateItemData;

private slots:
    void saveToModelData();

public slots:
    void reset();
    void loadFromModelData();
    void updateFromEditedModelData(int i, QVariantMap v);

signals:
    void addTriggered(QVariantMap item);
    void removeTriggered(QVariantMap item);

    void siteEdited(QVariantMap item);
};
