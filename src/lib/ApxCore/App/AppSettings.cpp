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
#include "AppSettings.h"
#include <App/App.h>
#include <App/AppDirs.h>
//=============================================================================
AppSettings::AppSettings(Fact *parent)
    : Fact(parent, "settings", tr("Preferences"), tr("Application settings"), Group | FlatModel) //root
{
    _instance = this;

    setIcon("settings");

    m_readOnly = false;
    m_settings = createSettings(this);

    f_interface = new Fact(this, "interface", tr("Interface"), "", Section);
    f_graphics = new Fact(this, "graphics", tr("Graphics"), "", Section);
    f_application = new Fact(this, "application", tr("Application"), "", Section);
    AppSettingFact *item;

    item = new AppSettingFact(m_settings,
                              f_interface,
                              "lang",
                              tr("Language"),
                              tr("Interface localization"),
                              Enum,
                              0);
    QStringList st;
    st << "default";
    QDir langp(AppDirs::lang());
    foreach (QFileInfo f, langp.entryInfoList(QStringList() << "*.qm"))
        st << f.baseName();
    item->setEnumStrings(st);

    item = new AppSettingFact(m_settings,
                              f_graphics,
                              "scale",
                              tr("Scale"),
                              tr("Fonts scale [-1..+1]"),
                              Float,
                              1.0);
    item->load();
    //item->setPrecision(1);
    //item->setMin(0.5);
    //item->setMax(2.0);
    scaleEvent.setInterval(1000);
    connect(item, &Fact::valueChanged, &scaleEvent, &DelayedEvent::schedule);
    connect(&scaleEvent, &DelayedEvent::triggered, this, [item]() {
        App::instance()->setScale(item->value().toDouble());
    });
    App::instance()->setScale(item->value().toDouble());

    item = new AppSettingFact(m_settings,
                              f_graphics,
                              "opengl",
                              tr("Accelerate graphics"),
                              tr("Enable OpenGL graphics when supported"),
                              Enum,
                              0);
    st.clear();
    st << "default";
    st << "OpenGL";
    st << "OpenGL ES 2.0";
    st << "OpenVG";
    item->setEnumStrings(st);

    item = new AppSettingFact(m_settings,
                              f_graphics,
                              "smooth",
                              tr("Smooth animations"),
                              tr("Enable animations and antialiasing"),
                              Bool,
                              true);

    item = new AppSettingFact(m_settings,
                              f_graphics,
                              "antialiasing",
                              tr("Antialiasing"),
                              tr("Enable antialiasing"),
                              Enum,
                              2);
    st.clear();
    st << "off";
    st << "minimum";
    st << "maximum";
    item->setEnumStrings(st);

    item = new AppSettingFact(m_settings,
                              f_graphics,
                              "effects",
                              tr("Effects"),
                              tr("Graphical effects level"),
                              Enum,
                              2);
    item->setEnumStrings(st);

    Fact *f = new Fact(f_graphics, "test", tr("Highlight instruments"), "", Bool);
    f->setValue(false);

    //load all settings
    AppSettingFact::loadSettings(this);

    App::jsync(this);

    //App::jsexec(QString("ui.%1=apx.settings.graphics.%1.value;").arg("scale"));
    //App::jsexec(QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });").arg("scale"));
    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("smooth"));
    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("antialiasing"));
    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("effects"));
    App::jsexec(
        QString("ui.__defineGetter__('%1', function(){ return apx.settings.graphics.%1.value; });")
            .arg("test"));
}
AppSettings *AppSettings::_instance = nullptr;
//=============================================================================
void AppSettings::saveValue(const QString &name, const QVariant &v, const QString &path)
{
    if (loadValue(name, path) == v)
        return;
    if (readOnly())
        return;
    QSettings *sx = m_settings;
    int grp = 0;
    foreach (const QString &s, path.split('/', QString::SkipEmptyParts)) {
        sx->beginGroup(s);
        grp++;
    }
    if (v.canConvert(QMetaType::QVariantList)) {
        const QVariantList &va = v.value<QVariantList>();
        sx->remove(name);
        sx->beginWriteArray(name, va.size());
        for (int i = 0; i < va.size(); ++i) {
            sx->setArrayIndex(i);
            const QVariant &vi = va.at(i);
            if (vi.canConvert(QMetaType::QVariantMap)) {
                const QVariantMap &vm = vi.value<QVariantMap>();
                foreach (QString key, vm.keys()) {
                    sx->setValue(key, vm.value(key));
                }
            } else {
                sx->setValue("value", vi);
            }
        }
        sx->endArray();
    } else {
        sx->setValue(name, v);
    }
    while (grp--)
        sx->endGroup();
}
QVariant AppSettings::loadValue(const QString &name,
                                const QString &path,
                                const QVariant &defaultValue)
{
    QSettings *sx = m_settings;
    int grp = 0;
    foreach (const QString &s, path.split('/', QString::SkipEmptyParts)) {
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
        if (vlist.isEmpty() && defaultValue.canConvert(QMetaType::QVariantList)) {
            v = defaultValue;
        } else {
            v = vlist;
        }
    } else {
        sx->endArray();
        v = sx->value(name, defaultValue);
    }
    while (grp--)
        sx->endGroup();
    return v;
}
QStringList AppSettings::allKeys(const QString &path)
{
    QSettings *sx = m_settings;
    int grp = 0;
    foreach (const QString &s, path.split('/', QString::SkipEmptyParts)) {
        sx->beginGroup(s);
        grp++;
    }
    QStringList v = sx->allKeys();
    while (grp--)
        sx->endGroup();
    return v;
}
void AppSettings::setReadOnly()
{
    m_readOnly = true;
}
bool AppSettings::readOnly()
{
    return m_readOnly;
}
//=============================================================================
void AppSettings::saveFile(const QString &name, const QString &v)
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
QString AppSettings::loadFile(const QString &name, const QString &defaultValue)
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
//=============================================================================
//=============================================================================
AppSettingFact::AppSettingFact(QSettings *settings,
                               Fact *parent,
                               QString name,
                               QString label,
                               QString descr,
                               Flags flags,
                               QVariant defaultValue)
    : Fact(parent, name, label, descr, flags)
    , m_settings(settings)
    , m_default(defaultValue)
{
    list.append(this);
}
QList<AppSettingFact *> AppSettingFact::list;
bool AppSettingFact::setValue(const QVariant &v)
{
    QVariant vx = v;
    if (vx.isNull()) { //reset to default
        vx = m_default;
    }
    if (!Fact::setValue(vx))
        return false;
    save();
    return true;
}
void AppSettingFact::load()
{
    m_value = m_default;
    m_settings->beginGroup(parentFact()->name());
    if (((!m_settings->contains(name())) || m_settings->value(name()).toString().isEmpty())
        && (!m_default.isNull())) {
        m_settings->setValue(name(), text());
    }
    Fact::setValue(m_settings->value(name(), m_default));
    m_settings->endGroup();
}
void AppSettingFact::save()
{
    if (AppSettings::instance()->readOnly())
        return;

    m_settings->beginGroup(parentFact()->name());
    m_settings->setValue(name(), text());
    m_settings->endGroup();
}
void AppSettingFact::loadSettings(Fact *group)
{
    foreach (AppSettingFact *i, AppSettingFact::list) {
        if (group && (!i->hasParent(group)))
            continue;
        i->load();
    }
}
//=============================================================================
//=============================================================================
