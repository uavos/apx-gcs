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

#include <QtCore>

class Fact;

class FactPropertyBinding : public QObject
{
    Q_OBJECT
public:
    explicit FactPropertyBinding(Fact *parent,
                                 Fact *src,
                                 const QString &name,
                                 FactPropertyBinding *src_binding);
    ~FactPropertyBinding();

    bool match(Fact *src, const QString &name) const;

    void block();
    void unblock();

private:
    Fact *_src;
    Fact *_dst;
    QString _name;
    QPointer<FactPropertyBinding> _src_binding;

    QList<QMetaObject::Connection> _clist;

    QMetaProperty _psrc;
    QMetaProperty _pdst;

    bool _blocked{false};

private slots:
    void propertyChanged();
};
