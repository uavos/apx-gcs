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
#include "AppPrefs.h"
#include "AppDirs.h"
#include "AppLog.h"

AppPrefs *AppPrefs::_instance = nullptr;

AppPrefs::AppPrefs(QObject *parent)
    : QObject(parent)
{
    _instance = this;

    //prefs
    QDir dir(AppDirs::prefs());
    if (!dir.exists())
        dir.mkpath(".");
    m_settings = new QSettings(this);
}

void AppPrefs::saveFile(const QString &name, const QString &v)
{
    QFile file(AppDirs::prefs().absoluteFilePath(name));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << file.errorString();
        return;
    }
    if (file.write(v.toUtf8().data()) != v.size()) {
        apxMsgW() << file.errorString();
    }
    file.close();
}
QString AppPrefs::loadFile(const QString &name, const QString &defaultValue)
{
    QFile file(AppDirs::prefs().absoluteFilePath(name));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        if (file.exists())
            apxMsgW() << file.errorString();
        return defaultValue;
    }
    QString s = file.readAll();
    file.close();
    if (s.isEmpty())
        s = defaultValue;
    return s;
}

void AppPrefs::saveValue(const QString &name, const QVariant &v, const QString &path)
{
    if (loadValue(name, path) == v)
        return;

    QSettings *sx = m_settings;
    int grp = 0;
    for (auto &s : path.split('/', Qt::SkipEmptyParts)) {
        sx->beginGroup(s);
        grp++;
    }

    sx->setValue(name, v);

    while (grp--)
        sx->endGroup();
}
void AppPrefs::removeValue(const QString &name, const QString &path)
{
    QSettings *sx = m_settings;
    int grp = 0;
    foreach (const QString &s, path.split('/', Qt::SkipEmptyParts)) {
        sx->beginGroup(s);
        grp++;
    }
    sx->remove(name);
    while (grp--)
        sx->endGroup();
}
QVariant AppPrefs::loadValue(const QString &name, const QString &path, const QVariant &defaultValue)
{
    QSettings *sx = m_settings;
    int grp = 0;
    foreach (const QString &s, path.split('/', Qt::SkipEmptyParts)) {
        sx->beginGroup(s);
        grp++;
    }
    QVariant v;
    int asz = sx->beginReadArray(name);
    if (asz > 0) {
        QVariantList vlist;
        for (int i = 0; i < asz; ++i) {
            sx->setArrayIndex(i);
            const QStringList keys = sx->allKeys();
            if (keys.size() > 1 || keys.first() != "value") {
                QVariantMap m;
                for (int j = 0; j < keys.size(); ++j) {
                    m[keys.at(j)] = sx->value(keys.at(j));
                }
                vlist.append(m);
            } else {
                vlist.append(sx->value("value"));
            }
        }
        sx->endArray();
        v = vlist;
    } else {
        sx->endArray();
        v = sx->value(name, defaultValue);
    }
    while (grp--)
        sx->endGroup();
    return v;
}
QStringList AppPrefs::allKeys(const QString &path)
{
    int grp = 0;
    foreach (const QString &s, path.split('/', Qt::SkipEmptyParts)) {
        m_settings->beginGroup(s);
        grp++;
    }
    QStringList v = m_settings->allKeys();
    while (grp--)
        m_settings->endGroup();
    return v;
}
