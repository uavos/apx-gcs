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
#ifndef AppPrefs_H
#define AppPrefs_H
//=============================================================================
#include <QtCore>
//=============================================================================
class AppPrefs : public QObject
{
    Q_OBJECT

public:
    explicit AppPrefs(QObject *parent = nullptr);

    static AppPrefs *instance() { return _instance; }

    static QSettings *settings() { return _instance->m_settings; }

    //called from qml to store json configs
    Q_INVOKABLE void saveFile(const QString &name, const QString &v);
    Q_INVOKABLE QString loadFile(const QString &name, const QString &defaultValue = QString());

    // called by facts to store persistent data
    // supports QVariantList, path can be '<grp1>/<grp2>/<etc>'
    Q_INVOKABLE void saveValue(const QString &name,
                               const QVariant &v,
                               const QString &path = QStringLiteral("qml"));

    Q_INVOKABLE void removeValue(const QString &name, const QString &path = QStringLiteral("qml"));

    Q_INVOKABLE QVariant loadValue(const QString &name,
                                   const QString &path = QStringLiteral("qml"),
                                   const QVariant &defaultValue = QVariant());

    Q_INVOKABLE QStringList allKeys(const QString &path);

private:
    static AppPrefs *_instance;

    QSettings *m_settings;
};
//=============================================================================
#endif
