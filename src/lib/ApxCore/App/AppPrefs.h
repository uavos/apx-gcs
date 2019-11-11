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
