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
#include "JsonHelpers.h"

QJsonObject json::add_content(QJsonObject jso, const QJsonObject &jso_add)
{
    // add missing fields to jso
    for (auto it = jso_add.begin(); it != jso_add.end(); ++it) {
        auto key = it.key();
        auto value = it.value();

        if (value.isNull() || value.isUndefined() || value.toString().isEmpty())
            continue; // skip empty values

        if (jso.contains(key))
            continue; // skip existing fields

        jso[key] = value;
    }
    return jso;
}

QJsonObject json::filter_names(QJsonObject jso, const QStringList &names, bool recursive)
{
    // recursively filter out null or empty object or empty string fields
    // and keep only those in fields array if set
    for (auto it = jso.begin(); it != jso.end();) {
        auto key = it.key();
        auto value = it.value();

        if (value.isNull() || value.isUndefined() || (!names.isEmpty() && !names.contains(key))) {
            it = jso.erase(it);
            continue;
        }

        if (value.isObject()) {
            auto jso_value = value.toObject();
            if (jso_value.isEmpty()) {
                it = jso.erase(it);
                continue;
            }
            if (recursive) {
                jso_value = json::filter_names(jso_value, names, recursive);
                if (jso_value.isEmpty()) {
                    it = jso.erase(it);
                    continue;
                }
                jso[key] = jso_value;
            }
        } else if (value.isArray() && value.toArray().isEmpty()) {
            it = jso.erase(it);
            continue;
        } else if (value.isString() && value.toString().isEmpty()) {
            it = jso.erase(it);
            continue;
        }

        // keep field
        ++it;
    }
    return jso;
}

QJsonObject json::fix_numbers(QJsonObject jso, const QStringList &names, bool recursive)
{
    // convert string numbers to numbers for keys in fields
    for (auto it = jso.begin(); it != jso.end(); ++it) {
        auto key = it.key();
        auto value = it.value();

        if (value.isString()) {
            if (!names.isEmpty() && !names.contains(key))
                continue;

            auto s = value.toString();
            bool ok = false;
            if (s.contains('.')) {
                auto d = s.toDouble(&ok);
                if (ok)
                    jso[key] = d;
            } else if (s.contains(':')) {
                auto t = QTime::fromString(s, "hh:mm:ss");
                if (t.isValid())
                    jso[key] = t.msecsSinceStartOfDay();
            } else if (QStringList({"true", "false", "yes", "no"}).contains(s)) {
                jso[key] = (s == "true" || s == "yes");
            } else {
                auto d = s.toLongLong(&ok);
                if (ok)
                    jso[key] = d;
            }
            continue;
        }

        if (!recursive)
            continue;

        if (value.isObject()) {
            jso[key] = json::fix_numbers(value.toObject(), names, recursive);
            // qDebug() << key << jso[key];
        }
        if (value.isArray()) {
            QJsonArray arr;
            for (auto v : value.toArray()) {
                if (v.isObject()) {
                    arr.append(json::fix_numbers(v.toObject(), names, recursive));
                } else {
                    arr.append(v);
                }
            }
            jso[key] = arr;
        }
    }
    return jso;
}

QJsonObject json::rename(QJsonObject jso, const QHash<QString, QString> &map)
{
    for (auto it = jso.begin(); it != jso.end();) {
        auto key = it.key();
        if (!map.contains(key)) {
            it++;
            continue;
        }

        jso[map[key]] = it.value();
        it = jso.erase(it);
    }
    return jso;
}

QJsonObject json::patch(const QJsonObject &orig, const QJsonObject &patch)
{
    QJsonObject result;
    for (auto it = orig.begin(); it != orig.end(); ++it) {
        auto key = it.key();
        auto value = it.value();

        if (value.isObject()) {
            if (!patch.contains(key)) {
                result[key] = value;
                continue;
            }
            auto sub = json::patch(orig[key].toObject(), value.toObject());
            if (!sub.isEmpty())
                result[key] = sub;
        } else if (value.isArray()) {
            if (!patch.contains(key)) {
                result[key] = value;
                continue;
            }
            auto orig_a = value.toArray();
            auto patch_a = patch[key].toArray();
            QJsonArray sub;
            for (int i = 0; i < orig_a.size(); ++i) {
                auto d = json::patch(orig_a.at(i).toObject(), patch_a.at(i).toObject());
                sub.append(d);
            }
            if (!sub.isEmpty())
                result[key] = sub;
        } else {
            // keep original value
            result[key] = value;
        }
    }
    return result;
}

QJsonObject json::diff(const QJsonObject &prev, const QJsonObject &next)
{
    QJsonObject diff;
    for (auto it = next.begin(); it != next.end(); ++it) {
        auto pit = prev.find(it.key());
        if (pit == prev.end()) {
            // new key added
            diff[it.key()] = it.value();
            // qDebug() << "new key" << it.key() << it.value();
        } else if (pit.value() != it.value()) {
            // qDebug() << "value diff" << it.key() << pit.value() << it.value();
            if (pit.value().isObject() && it.value().isObject()) {
                diff[it.key()] = json::diff(pit.value().toObject(), it.value().toObject());
            } else if (pit.value().isArray() && it.value().isArray()) {
                QJsonArray da;
                auto pa = pit.value().toArray();
                auto na = it.value().toArray();
                for (int i = 0; i < na.size(); ++i) {
                    da.append(json::diff(pa.at(i).toObject(), na.at(i).toObject()));
                }
                diff[it.key()] = da;
            } else {
                diff[it.key()] = it.value();
            }
        }
    }
    return diff;
}
