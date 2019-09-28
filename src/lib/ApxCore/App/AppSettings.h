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
#ifndef AppSettings_H
#define AppSettings_H
//=============================================================================
#include <ApxDirs.h>
#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class AppSettings : public Fact
{
    Q_OBJECT
public:
    explicit AppSettings(Fact *parent = nullptr);

    static AppSettings *instance() { return _instance; }

    Fact *f_interface;
    Fact *f_graphics;
    Fact *f_application;

    static QSettings *settings() { return _instance->m_settings; }

    static QSettings *createSettings(QObject *parent = nullptr)
    {
        QDir spath(ApxDirs::prefs());
        if (!spath.exists())
            spath.mkpath(".");
        return new QSettings(spath.absoluteFilePath(QCoreApplication::applicationName() + ".ini"),
                             QSettings::IniFormat,
                             parent);
    }

    static QVariant value(const QString &namePath) { return _instance->findValue(namePath); }
    static bool setValue(QString namePath, const QVariant &v)
    {
        Fact *f = _instance->findChild(namePath);
        if (!f)
            return false;
        return f->setValue(v);
    }

    Q_INVOKABLE void saveValue(const QString &name,
                               const QVariant &v,
                               const QString &path = QStringLiteral("qml"));
    Q_INVOKABLE QVariant loadValue(const QString &name,
                                   const QString &path = QStringLiteral("qml"),
                                   const QVariant &defaultValue = QVariant());
    Q_INVOKABLE QStringList allKeys(const QString &path);

    Q_INVOKABLE void saveFile(const QString &name, const QString &v);
    Q_INVOKABLE QString loadFile(const QString &name, const QString &defaultValue = QString());

    bool readOnly();

public slots:
    void setReadOnly();

private:
    static AppSettings *_instance;
    QSettings *m_settings;
    bool m_readOnly;
    DelayedEvent scaleEvent;
};
//=============================================================================
class AppSettingFact : public Fact
{
    Q_OBJECT
public:
    explicit AppSettingFact(QSettings *settings,
                            Fact *parent,
                            QString name,
                            QString label,
                            QString descr,
                            Fact::Flags flags,
                            QVariant defaultValue = QVariant());

    static QList<AppSettingFact *> list;
    static void loadSettings(Fact *group);

    void load();
    void save();

    //Fact override
    bool setValue(const QVariant &v);

private:
    QSettings *m_settings;
    QVariant m_default;
};
//=============================================================================
#endif
