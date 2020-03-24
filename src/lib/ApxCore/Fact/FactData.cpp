﻿/*
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
#include "FactData.h"

#include <App/AppPrefs.h>
#include <App/AppRoot.h>
#include <cmath>

FactData::FactData(
    QObject *parent, const QString &name, const QString &title, const QString &descr, Flags flags)
    : FactBase(parent, name, flags)
{
    connect(this, &FactData::valueChanged, this, &FactData::updateText);
    connect(this, &FactData::precisionChanged, this, &FactData::updateText);
    connect(this, &FactData::enumStringsChanged, this, &FactData::updateText, Qt::QueuedConnection);

    connect(this, &FactData::dataTypeChanged, this, &FactData::defaults);
    connect(this, &FactData::dataTypeChanged, this, &FactData::valueChanged);

    connect(this, &FactData::optionsChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::pathChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::defaultValueChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::dataTypeChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::enumStringsChanged, this, &FactData::getPresistentValue);

    setTitle(title);
    setDescr(descr);

    setDataType(Flag(uint(flags) & DataMask));
}

FactBase::Flag FactData::dataType() const
{
    return m_dataType;
}
void FactData::setDataType(FactBase::Flag v)
{
    v = static_cast<Flag>(v & DataMask);
    if (m_dataType == v)
        return;
    m_dataType = v;

    if (v == Count) {
        connect(this, &FactBase::sizeChanged, this, &FactData::valueChanged);
    } else {
        disconnect(this, &FactBase::sizeChanged, this, &FactData::valueChanged);
    }

    emit dataTypeChanged();
}

QVariant FactData::value(void) const
{
    if (dataType() == Count && m_value.isNull()) {
        return size() > 0 ? QString::number(size()) : QString();
    }
    return m_value;
}

bool FactData::setValue(const QVariant &v)
{
    if (!updateValue(v))
        return false;

    if (options() & ModifiedTrack)
        setModified(backupValue() != m_value);

    if (m_options & PersistentValue)
        savePresistentValue();

    emit valueChanged();
    return true;
}
bool FactData::updateValue(const QVariant &v)
{
    //filter the same
    const QVariant &v_prev = value();
    /*if (_check_type(v, QMetaType::QByteArray)) {
        if (_check_type(v_prev, QMetaType::QByteArray) && v_prev.toByteArray() == v.toByteArray())
            return false;
    } else if (v_prev == v)
        return false;*/

    if (v_prev == v)
        return false;

    switch (dataType()) {
    case Enum: {
        //qDebug()<<"set"<<path()<<vx<<"->"<<ev;
        int enumIndex = enumValue(v);
        if (enumIndex < 0) {
            if (m_enumStrings.size() != 2)
                return false;
            enumIndex = _string_to_bool(v.toString());
            if (enumIndex < 0)
                return false;
        }
        if (v_prev.toInt() == enumIndex)
            return false;
        //qDebug() << path() << m_value << enumIndex;
        m_value = QVariant::fromValue(enumIndex);
    } break;
    case Bool: {
        int enumIndex = enumValue(v);
        if (enumIndex < 0) {
            enumIndex = _string_to_bool(v.toString());
            if (enumIndex < 0)
                enumIndex = 0;
        }
        if (v_prev.toBool() == enumIndex)
            return false;
        m_value = QVariant::fromValue(enumIndex);
    } break;
    case Text: {
        int enumIndex = enumValue(v);
        const QString &s = enumIndex < 0 ? v.toString() : enumText(enumIndex);
        if (v_prev.toString() == s)
            return false;
        m_value = QVariant::fromValue(s);
    } break;
    case Int: {
        int enumIndex = enumValue(v);
        if (enumIndex >= 0) {
            m_value = enumText(enumIndex);
            break;
        }
        if (m_enumStrings.size() > 1)
            return false;

        long long vi = 0;
        bool ok = false;
        if (!_check_int(v)) {
            QString s = v.toString();
            if (units() == "time" && s.contains(':')) {
                vi = static_cast<long long>(AppRoot::timeFromString(s));
                ok = true;
            }
            if (!ok && units() == "hex") {
                vi = s.toLongLong(&ok, 16);
            }
            if (!ok) {
                vi = s.toLongLong(&ok);
                if (!ok)
                    vi = static_cast<long long>(round(s.toDouble(&ok)));
                if (!ok)
                    vi = s.toLongLong(&ok, 16);
            }
            if (!ok)
                return false;
        } else {
            vi = v.toLongLong();
        }
        if (!m_min.isNull()) {
            long long m = m_min.toLongLong();
            if (vi < m)
                vi = m;
        }
        if (!m_max.isNull()) {
            long long m = m_max.toLongLong();
            if (vi > m)
                vi = m;
        }
        if (v_prev.toLongLong() == vi)
            return false;
        m_value = QVariant::fromValue(vi);
    } break;
    case Float: {
        qreal vf;
        if (!_check_type(v, QMetaType::Double)) {
            const QString &s = v.toString();
            if (units() == "lat") {
                vf = AppRoot::latFromString(s);
            } else if (units() == "lon") {
                vf = AppRoot::lonFromString(s);
            } else {
                bool ok = false;
                vf = s.toDouble(&ok);
                if (!ok) {
                    int i = _string_to_int(s);
                    if (i < 0)
                        return false;
                    vf = i;
                }
                if ((!m_min.isNull()) && vf < m_min.toDouble())
                    vf = m_min.toDouble();
                if ((!m_max.isNull()) && vf > m_max.toDouble())
                    vf = m_max.toDouble();
            }
        } else
            vf = v.toDouble();

        if (v_prev.toDouble() == vf)
            return false;

        m_value = QVariant::fromValue(vf);
    } break;
    case MandalaID: {
        quint16 vuid = stringToMandala(v.toString().trimmed());
        if (v_prev.toUInt() == vuid)
            return false;
        m_value = QVariant::fromValue(vuid);
    } break;
    case Script: {
        const QString &s = v.toString();
        if (v_prev.toString() == s)
            return false;
        m_value = QVariant::fromValue(s);
    } break;
    default: {
        int enumIndex = enumValue(v);
        if (enumIndex >= 0) {
            if (v_prev.toInt() == enumIndex)
                return false;
            m_value = QVariant::fromValue(enumIndex);
            break;
        }
        if (toText(v) == text())
            return false;
        m_value = v;
    } break;
    }
    return true;
}
//=============================================================================
int FactData::enumValue(const QVariant &v) const
{
    if (m_enumStrings.isEmpty())
        return -1;

    int idx = -1;
    if (_check_int(v))
        idx = v.toInt();
    else {
        idx = m_enumStrings.indexOf(v.toString());
        if (idx >= 0)
            return idx;
    }
    if (idx < 0)
        idx = _string_to_int(v.toString());
    if (idx < 0 || idx >= m_enumStrings.size())
        return -1;
    return idx;
}
QString FactData::enumText(int idx) const
{
    return m_enumStrings.value(idx);
}

int FactData::_string_to_bool(QString s)
{
    s = s.toLower();
    if (s == "true" || s == "on" || s == "yes")
        return 1;
    else if (s == "false" || s == "off" || s == "no")
        return 0;
    int v = _string_to_int(s);
    if (v < 0)
        return -1;
    return v > 0 ? 1 : 0;
}
int FactData::_string_to_int(const QString &s)
{
    bool ok = false;
    int v = s.toInt(&ok);
    if (ok)
        return v;
    v = static_cast<int>(s.toDouble(&ok));
    if (ok)
        return v;
    return -1;
}
bool FactData::_check_type(const QVariant &v, QMetaType::Type t)
{
    return static_cast<QMetaType::Type>(v.type()) == t;
}
bool FactData::_check_int(const QVariant &v)
{
    return _check_type(v, QMetaType::Int) || _check_type(v, QMetaType::UInt)
           || _check_type(v, QMetaType::UChar) || _check_type(v, QMetaType::UShort)
           || _check_type(v, QMetaType::ULongLong);
}

//=============================================================================
static double cint(double x)
{
    double i;
    if (std::modf(x, &i) >= .5)
        return x >= 0 ? std::ceil(x) : std::floor(x);
    else
        return x < 0 ? std::ceil(x) : std::floor(x);
}
QString FactData::toText(const QVariant &v) const
{
    Flag t = dataType();
    if (t == Script) {
        return v.toString();
    }
    if ((t != Group) && (!m_enumStrings.isEmpty())) {
        int ev = enumValue(v);
        if (ev >= 0)
            return enumText(ev);
        if (t == Enum)
            return v.toString();
    }
    if (t == Int) {
        if (units() == "hex")
            return QString::number(v.toUInt(), 16).toUpper();
        if (units() == "time") {
            return AppRoot::timeToString(v.toUInt(), true);
        }
        return QString::number(v.toString().toLongLong());
    }
    if (t == Bool) {
        return QVariant(v.toBool()).toString();
    }
    if (t == MandalaID) {
        return mandalaToString(static_cast<quint16>(v.toUInt()));
    }
    if (t == Float) {
        if (units() == "lat") {
            return AppRoot::latToString(v.toDouble());
        }
        if (units() == "lon") {
            return AppRoot::lonToString(v.toDouble());
        }
    }
    if (_check_type(v, QMetaType::QByteArray))
        return v.toByteArray().toHex().toUpper();
    if (_check_type(v, QMetaType::Double)) {
        double vf = v.toDouble();
        if (m_precision > 0) {
            double p = std::pow(10.0, m_precision);
            vf = cint(vf * p) / p;
        }
        if (vf == 0.0)
            return "0";
        QString s;
        s = QString("%1").arg(vf, 0, 'f', m_precision > 0 ? m_precision : 8);
        if (s.contains('.')) {
            if (m_precision > 0)
                s = s.left(s.indexOf('.') + 1 + m_precision);
            while (s.endsWith('0'))
                s.chop(1);
            if (s.endsWith('.'))
                s.chop(1);
        }
        return s;
        //return QString::asprintf("%f").arg(vf,0,'g',m_precision);
    }
    return v.toString();
}
//=============================================================================
bool FactData::isZero() const
{
    if (treeType() == Group) {
        for (int i = 0; i < size(); i++) {
            FactData *f = child(i);
            if (!f->isZero())
                return false;
        }
        return true;
    }
    Flag t = dataType();
    const QVariant &v = value();
    if (v.isNull())
        return true;
    if (t == Text || t == Script)
        return v.toString().isEmpty();
    if (t == Float)
        return v.toDouble() == 0.0;
    const QString &s = text().trimmed();
    if (s.isEmpty())
        return true;
    if (s == "0")
        return true;
    if (v.toInt() == 0)
        return true;
    return false;
}
//=============================================================================
//=============================================================================
bool FactData::modified() const
{
    return m_modified;
}
void FactData::setModified(const bool &v)
{
    if (m_modified == v)
        return;
    m_modified = v;
    emit modifiedChanged();

    if (!(options() & (ModifiedTrack | ModifiedGroup)))
        return;

    Fact *p = parentFact();
    if (!p)
        return;
    if (!(p->options() & (ModifiedTrack | ModifiedGroup)))
        return;

    for (auto i : p->facts()) {
        if (i->modified()) {
            p->setModified(true);
            return;
        }
    }
    p->setModified(false);
}
int FactData::precision(void) const
{
    return m_precision;
}
void FactData::setPrecision(const int &v)
{
    if (m_precision == v)
        return;
    m_precision = v;
    emit precisionChanged();
}
QVariant FactData::min(void) const
{
    return m_min;
}
void FactData::setMin(const QVariant &v)
{
    if (m_min == v)
        return;
    m_min = v;
    emit minChanged();
    if (v.isNull())
        return;
    if (value().toDouble() < m_min.toDouble())
        setValue(m_min);
}
QVariant FactData::max(void) const
{
    return m_max;
}
void FactData::setMax(const QVariant &v)
{
    if (m_max == v)
        return;
    m_max = v;
    emit maxChanged();
    if (v.isNull())
        return;
    if (value().toDouble() > m_max.toDouble())
        setValue(m_max);
}
QString FactData::title(void) const
{
    return m_title.isEmpty() ? name() : m_title;
}
void FactData::setTitle(const QString &v)
{
    QString s = v.trimmed();
    if (m_title == s)
        return;
    m_title = s;
    emit titleChanged();
}
QString FactData::descr(void) const
{
    return m_descr;
}
void FactData::setDescr(const QString &v)
{
    QString s = v.trimmed();
    if (m_descr == s)
        return;
    m_descr = s;
    emit descrChanged();
}
QString FactData::text() const
{
    return m_text;
}
void FactData::setText(const QString &v)
{
    if (m_text == v)
        return;
    m_text = v;
    emit textChanged();
}
void FactData::updateText()
{
    setText(toText(value()));
}
const QStringList &FactData::enumStrings() const
{
    return m_enumStrings;
}
void FactData::setEnumStrings(const QStringList &v)
{
    if (m_enumStrings == v)
        return;
    m_enumStrings = v;
    emit enumStringsChanged();
}
void FactData::setEnumStrings(const QMetaEnum &v)
{
    QStringList st;
    for (int i = 0; i < v.keyCount(); ++i) {
        int vi = v.value(i);
        st.append(v.valueToKey(vi));
    }
    setEnumStrings(st);
}
QString FactData::units() const
{
    return m_units;
}
void FactData::setUnits(const QString &v)
{
    if (m_units == v)
        return;
    m_units = v;
    emit unitsChanged();
}
QVariant FactData::defaultValue(void) const
{
    return m_defaultValue;
}
void FactData::setDefaultValue(const QVariant &v)
{
    if (m_defaultValue == v)
        return;
    m_defaultValue = v;
    emit defaultValueChanged();
}
//=============================================================================
QString FactData::mandalaToString(quint16 uid) const
{
    Q_UNUSED(uid)
    return QString();
}
quint16 FactData::stringToMandala(const QString &s) const
{
    Q_UNUSED(s)
    return 0;
}
//=============================================================================
void FactData::copyValuesFrom(const FactData *item)
{
    for (int i = 0; i < size(); ++i) {
        FactData *dest = child(i);
        FactData *src = item->child(dest->name());
        if (!src)
            continue;
        if ((dest->treeType() | dest->dataType()) != (src->treeType() | src->dataType()))
            continue;
        if (dest->treeType() == Group) {
            dest->copyValuesFrom(src);
        } else {
            dest->setValue(src->value());
        }
    }
}

QVariant FactData::backupValue(void) const
{
    return m_backupValue;
}
void FactData::setBackupValue(const QVariant &v)
{
    if (options() & ModifiedTrack)
        setModified(v != m_value);

    if (m_backupValue == v)
        return;
    m_backupValue = v;
    emit backupValueChanged();
}

void FactData::backup()
{
    for (auto i : facts()) {
        i->backup();
    }

    setModified(false);

    if (treeType() == Group)
        return;

    setBackupValue(m_value);
}
void FactData::restore()
{
    if (!modified())
        return;
    if (size()) {
        for (int i = 0; i < size(); ++i) {
            child(i)->restore();
        }
    }
    if (treeType() != Group) {
        setValue(backupValue());
    }
    setModified(false);
}
void FactData::defaults()
{
    if (m_value.isNull()) {
        switch (dataType()) {
        case Float:
            m_value = 0.0;
            break;
        case Int:
            m_value = 0;
            break;
        case Bool:
            m_value = false;
            break;
        case Enum:
            m_value = 0;
            break;
        case MandalaID:
            m_value = 0;
            break;
        default:
            m_value = QVariant();
        }
    }
}

void FactData::getPresistentValue()
{
    if (m_options & PersistentValue)
        loadPresistentValue();
}
void FactData::loadPresistentValue()
{
    if (!parentFact())
        return;
    QVariant v = defaultValue();
    if (m_options & SystemSettings) {
        v = QSettings().value(path(), v);
    } else {
        v = AppPrefs::instance()->loadValue(name(), prefsGroup(), v);
    }
    if (!updateValue(v))
        return;
    emit valueChanged();
}
void FactData::savePresistentValue()
{
    QString s = toText(m_value);
    if (dataType() == MandalaID && s.isEmpty())
        s = "disabled";
    const bool rm = m_value == defaultValue() || s == defaultValue() || s == toText(defaultValue());
    if (m_options & SystemSettings) {
        if (rm)
            QSettings().remove(path());
        else
            QSettings().setValue(path(), s);
    } else {
        if (rm)
            AppPrefs::instance()->removeValue(name(), prefsGroup());
        else
            AppPrefs::instance()->saveValue(name(), s, prefsGroup());
    }
}
QString FactData::prefsGroup() const
{
    QStringList p = pathStringList();
    p.removeLast();
    return p.join('.');
}
