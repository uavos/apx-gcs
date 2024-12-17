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

void json::save(QString fileName, const QJsonValue &jsv)
{
    // default to downloads folder
    auto fi = QFileInfo(fileName);
    if (fi.suffix() != "json") {
        fileName = fileName + ".json";
        fi = QFileInfo(fileName);
    }

    if (fi.isRelative()) {
        fileName = QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
                       .absoluteFilePath(fi.fileName());
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "failed to open file" << fileName;
        return;
    }
    if (jsv.isObject())
        file.write(QJsonDocument(jsv.toObject()).toJson(QJsonDocument::Indented));
    else if (jsv.isArray())
        file.write(QJsonDocument(jsv.toArray()).toJson(QJsonDocument::Indented));
    else
        file.write(jsv.toVariant().toString().toUtf8());
}

static QJsonValueRef _remove_empty(QJsonValueRef value, bool remove_zeroes)
{
    do {
        if (value.isNull() || value.isUndefined())
            break;

        if (value.isObject()) {
            value = json::remove_empty(value.toObject(), remove_zeroes);
            if (value.toObject().isEmpty())
                break;
            return value;
        }
        if (value.isArray()) {
            value = json::remove_empty(value.toArray(), remove_zeroes);
            if (value.toArray().isEmpty())
                break;
            return value;
        }

        if (value.isString()) {
            if (value.toString().isEmpty())
                break;
            return value;
        }
        if (remove_zeroes) {
            if (value.isDouble()) {
                if (value.toDouble() == 0)
                    break;
                return value;
            }
            if (value.isBool()) {
                if (!value.toBool())
                    break;
                return value;
            }
        }

        // no change - return as is
        return value;
    } while (0);

    // set to null to be further removed
    value = QJsonValue();
    return value;
}

QJsonObject json::remove_empty(QJsonObject jso, bool remove_zeroes)
{
    // recursively filter out null or empty object or empty string fields
    for (auto it = jso.begin(); it != jso.end();) {
        auto jsv = _remove_empty(it.value(), remove_zeroes);
        if (jsv.isNull()) {
            jso.erase(it);
            continue;
        }
        // keep field
        it++;
    }
    return jso;
}

QJsonArray json::remove_empty(QJsonArray jsa, bool remove_zeroes)
{
    // recursively filter out null or empty object or empty string fields
    uint cnt = 0;
    for (auto v : jsa) {
        _remove_empty(v, remove_zeroes);
        if (!v.isNull())
            cnt++;
    }
    return cnt > 0 ? jsa : QJsonArray();
}

static QJsonValueRef _fix_number(QJsonValueRef value,
                                 const QStringList &names = {},
                                 bool recursive = true)
{
    if (recursive) {
        if (value.isObject()) {
            value = json::fix_numbers(value.toObject(), names, recursive);
            return value;
        } else if (value.isArray()) {
            value = json::fix_numbers(value.toArray(), names, recursive);
            return value;
        }
    }

    if (!value.isString())
        return value;

    auto s = value.toString();
    bool ok = false;
    if (s.contains('.')) {
        auto d = s.toDouble(&ok);
        if (ok)
            value = d;
    } else if (s.contains(':')) {
        auto t = QTime::fromString(s, "hh:mm:ss");
        if (t.isValid())
            value = t.msecsSinceStartOfDay();
    } else if (QStringList({"true", "false", "yes", "no"}).contains(s.toLower())) {
        value = (s == "true" || s == "yes");
    } else {
        auto d = s.toLongLong(&ok);
        if (ok)
            value = d;
    }
    return value;
}

QJsonObject json::fix_numbers(QJsonObject jso, const QStringList &names, bool recursive)
{
    // convert string numbers to numbers for keys in fields
    for (auto it = jso.begin(); it != jso.end(); ++it) {
        if (!names.isEmpty() && !names.contains(it.key()))
            continue;
        _fix_number(it.value(), names, recursive);
    }
    return jso;
}
QJsonArray json::fix_numbers(QJsonArray jsa, const QStringList &names, bool recursive)
{
    // convert string numbers to numbers for keys in fields
    for (auto v : jsa) {
        _fix_number(v, names, recursive);
    }
    return jsa;
}
QJsonValue json::fix_number(QJsonValue jsv)
{
    QJsonArray jsa{jsv}; // temporary array
    _fix_number(jsa[0]); // modify reference in place
    return jsa.first();
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

static QJsonValue _diff(QJsonValue prev, QJsonValue next)
{
    if (prev.isObject()) {
        auto jso = json::diff(prev.toObject(), next.toObject());
        return jso.isEmpty() ? QJsonValue() : jso;
    }
    if (prev.isArray()) {
        auto jsa = json::diff(prev.toArray(), next.toArray());
        return jsa.isEmpty() ? QJsonValue() : jsa;
    }

    if (next.isNull() || next.isUndefined() || prev == next)
        return QJsonValue();

    if (prev.isNull() || prev.isUndefined()) {
        return next; // not null or undefined or empty
    }

    return next;
}

QJsonObject json::diff(const QJsonObject &prev, const QJsonObject &next)
{
    QJsonObject diff;
    for (auto next_it = next.begin(); next_it != next.end(); ++next_it) {
        const auto key = next_it.key();

        auto v = _diff(prev.value(key), next_it.value());
        if (v.isNull())
            continue;

        diff[key] = v;
    }
    return diff;
}

QJsonArray json::diff(const QJsonArray &prev, const QJsonArray &next)
{
    QJsonArray diff;
    for (int i = 0; i < next.size(); ++i) {
        auto value = next.at(i);
        auto v = _diff(i < prev.size() ? prev.at(i) : QJsonValue(), value);
        diff.append(v);
    }
    // remove null values from tail
    while (!diff.isEmpty() && diff.last().isNull())
        diff.removeLast();
    return diff;
}

static QJsonValue _merge(QJsonValue orig, const QJsonValue patch)
{
    if (orig.isObject()) {
        auto jso = json::merge(orig.toObject(), patch.toObject());
        return jso.isEmpty() ? QJsonValue() : jso;
    }
    if (orig.isArray()) {
        auto jsa = json::merge(orig.toArray(), patch.toArray());
        return jsa.isEmpty() ? QJsonValue() : jsa;
    }

    // filter out empty values from original object
    // will add them from patch on next iteration
    bool is_orig_missing = orig.isNull() || orig.isUndefined()
                           || (orig.isObject() && orig.toObject().isEmpty())
                           || (orig.isArray() && orig.toArray().isEmpty());

    bool is_patch_missing = patch.isNull() || patch.isUndefined()
                            || (patch.isObject() && patch.toObject().isEmpty())
                            || (patch.isArray() && patch.toArray().isEmpty());

    if (is_orig_missing) {
        if (is_patch_missing)
            return QJsonValue();
        return patch;
    }
    // orig is not empty
    if (is_patch_missing) {
        return orig;
    }
    // replace value with patch
    return patch;
}

QJsonObject json::merge(QJsonObject orig, const QJsonObject &patch)
{
    QJsonObject result;
    for (auto it = orig.begin(); it != orig.end(); ++it) {
        const auto key = it.key();
        const auto v = _merge(it.value(), patch.value(key));
        if (v.isNull() || v.isUndefined())
            continue;
        result[key] = v;
    }
    return result;
}

QJsonArray json::merge(QJsonArray orig, const QJsonArray &patch)
{
    // add missing array elements
    while (patch.size() > orig.size())
        orig.append(patch.at(orig.size()));

    QJsonArray result;
    int idx = 0;
    for (auto value : orig) {
        auto t = value.type();
        auto v = _merge(value, patch.at(idx++));
        if (v.isNull() || v.isUndefined())
            v = QJsonValue(t); // keep original type
        result.append(v);      // always keep original array length
    }

    return result;
}
