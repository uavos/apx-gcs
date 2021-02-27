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

#include <QJsonDocument>
#include <QtCore>

class ShareImport : public QObject
{
    Q_OBJECT
public:
    explicit ShareImport(QString name, QString type, QObject *parent = nullptr);

    QString name() const { return _name; }
    QString type() const { return _type; }

    QByteArray loadData(QString fileName);

protected:
    QString _name;
    QString _type;

signals:
    void imported(QString fileName, QString hash, QString title);
};
