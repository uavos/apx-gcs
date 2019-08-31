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
#ifndef NodeField_H
#define NodeField_H
//=============================================================================
#include "NodesBase.h"
#include <QtCore>
class NodeItem;
class PawnCompiler;
//=============================================================================
class NodeField : public NodesBase
{
    Q_OBJECT

public:
    explicit NodeField(NodeItem *node,
                       quint16 id,
                       int dtype,
                       const QString &name,
                       const QString &title,
                       const QString &descr,
                       const QString &units,
                       const QStringList &opts,
                       const QStringList &groups,
                       NodeField *parentField = nullptr);

    quint16 id;
    int dtype;
    QStringList groups;

    //Fact override
    bool setValue(const QVariant &v) override;
    void setModified(const bool &v, const bool &recursive = false) override;

    QVariant uploadableValue(void) const;

    void hashData(QCryptographicHash *h) const override;

    NodeItem *node;
    PawnCompiler *pawncc;

    //values used for storage and share
    QString toString() const;
    void fromString(const QString &s);

private:
    NodeField *parentField;

    QByteArray scriptCodeSave;

private slots:
    void updateStatus();
    void validateData();
};
//=============================================================================
#endif
