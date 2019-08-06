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
#ifndef Sites_H
#define Sites_H
//=============================================================================
#include "LookupSites.h"
#include "SiteEdit.h"
#include <Fact/Fact.h>
#include <QtCore>
#include <QtLocation>
//=============================================================================
class Sites : public Fact
{
    Q_OBJECT

public:
    explicit Sites(Fact *parent = nullptr);

    Q_INVOKABLE void createEditor(QVariantMap item);
    Q_INVOKABLE void destroyEditor(QVariantMap item);

    LookupSites *f_lookup;

private:
    SiteEdit *f_add;
    SiteEdit *f_edit;

private slots:
    void appLoaded();
    void syncEditorFromModel();

    void dbAddSite(QVariantMap item);
    void dbRemoveSite(QVariantMap item);
    void dbUpdateSite(QVariantMap item);
};
//=============================================================================
#endif
