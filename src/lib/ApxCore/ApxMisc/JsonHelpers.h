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

namespace json {

void save(QString fileName, const QJsonValue &jsv);

QJsonObject fix_numbers(QJsonObject jso, const QStringList &names = {}, bool recursive = true);
QJsonArray fix_numbers(QJsonArray jsa, const QStringList &names = {}, bool recursive = true);
QJsonValue fix_number(QJsonValue jsv);

QJsonObject remove_empty(QJsonObject jso_src, bool remove_zeroes = false);
QJsonArray remove_empty(QJsonArray jsa_src, bool remove_zeroes = false);

QJsonObject rename(QJsonObject jso, const QHash<QString, QString> &map);

QJsonObject diff(const QJsonObject &prev, const QJsonObject &next);
QJsonArray diff(const QJsonArray &prev, const QJsonArray &next);

QJsonObject merge(QJsonObject orig, const QJsonObject &patch);
QJsonArray merge(QJsonArray orig, const QJsonArray &patch);

} // namespace json
