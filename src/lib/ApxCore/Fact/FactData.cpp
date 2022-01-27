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
#include "FactData.h"

#include <App/AppPrefs.h>
#include <App/AppRoot.h>
#include <cmath>

FactData::FactData(
    QObject *parent, const QString &name, const QString &title, const QString &descr, Flags flags)
    : FactBase(parent, name, flags)
{
    connect(this, &FactData::valueChanged, this, &FactData::updateValueText);
    connect(this, &FactData::precisionChanged, this, &FactData::updateValueText);
    connect(this, &FactData::enumStringsChanged, this, &FactData::updateValueText);
    connect(this, &FactData::unitsChanged, this, &FactData::updateValueText);

    connect(this, &FactData::valueTextChanged, this, &FactData::updateText);
    connect(this, &FactData::unitsChanged, this, &FactData::updateText);

    connect(this, &FactData::dataTypeChanged, this, &FactData::valueChanged);

    connect(this, &FactData::optionsChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::pathChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::defaultValueChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::dataTypeChanged, this, &FactData::getPresistentValue);
    connect(this, &FactData::enumStringsChanged, this, &FactData::getPresistentValue);

    setTitle(title);
    setDescr(descr);

    setDataType(Flag(uint(flags) & DataMask));

    updateValueText();
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
        connect(this, &FactBase::sizeChanged, this, &FactData::valueChanged, Qt::UniqueConnection);
    } else {
        disconnect(this, &FactBase::sizeChanged, this, &FactData::valueChanged);
    }

    emit dataTypeChanged();
}

QVariant FactData::value(void) const
{
    return _type_value(m_value);
}

bool FactData::setValue(const QVariant &v)
{
    if (!updateValue(v))
        return false;

    if (options() & ModifiedTrack)
        setModified(backupValue().toString() != value().toString());

    if (m_options & PersistentValue)
        savePresistentValue();

    emit valueChanged();
    return true;
}
bool FactData::updateValue(const QVariant &v)
{
    //filter the same
    const QVariant &v_prev = value();

    if (v_prev == v)
        return false;

    switch (dataType()) {
    default:
        m_value = v;
        break;
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
        QString s;
        if (m_units.isEmpty()) {
            int enumIndex = enumValue(v);
            s = enumIndex < 0 ? v.toString() : enumText(enumIndex);
        } else {
            s = v.toString();
        }

        if (v_prev.toString() == s)
            return false;
        m_value = QVariant::fromValue(s);
    } break;
    case Int: {
        if (units() == "mandala") {
            quint32 bind_raw;
            do {
                if (v.isNull()) {
                    bind_raw = 0;
                    break;
                }
                bool ok = false;
                bind_raw = v.toUInt(&ok);
                if (_check_int(v) || ok) {
                    if (mandalaToString(bind_raw).isEmpty()) {
                        bind_raw = 0;
                    }
                    break;
                }
                bind_raw = stringToMandala(v.toString().trimmed());
            } while (0);
            const QVariant &vx = bind_raw ? QVariant::fromValue(bind_raw) : QVariant();
            if (v_prev == vx)
                return false;
            m_value = vx;
            break;
        }

        QVariant vx = v;
        QString s = vx.toString();
        QString u = units();
        if (!u.isEmpty() && s.endsWith(u)) {
            s = s.left(s.size() - u.size()).trimmed();
            vx = s;
        }

        int enumIndex = enumValue(vx);
        if (enumIndex >= 0) {
            m_value = enumIndex;
            break;
        }
        if (m_enumStrings.size() > 1)
            return false;

        long long vi = 0;
        bool ok = false;

        if (u == "hex") {
            vi = s.toLongLong(&ok, 16);
            if (!ok)
                return false;
        } else if (u == "s") {
            vi = static_cast<long long>(AppRoot::timeFromString(s, true));
            ok = true;
        } else if (u == "min") {
            vi = static_cast<long long>(AppRoot::timeFromString(s, false));
            ok = true;
        } else if (!_check_int(vx)) {
            vi = s.toLongLong(&ok);
            if (!ok)
                vi = static_cast<long long>(round(s.toDouble(&ok)));
            if (!ok)
                vi = s.toLongLong(&ok, 16);
            if (!ok)
                return false;
        } else {
            vi = vx.toLongLong();
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
        if (!(_check_type(v, QMetaType::Double) || _check_type(v, QMetaType::Float))) {
            QString s = v.toString();
            QString u = units();
            if (!u.isEmpty() && s.endsWith(u)) {
                s = s.left(s.size() - u.size()).trimmed();
            }
            if (u == "lat") {
                vf = AppRoot::latFromString(s);
            } else if (u == "lon") {
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
    }
    return true;
}

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
QString FactData::_string_with_units(const QString &v) const
{
    const QString u = units();
    if (u.isEmpty())
        return v;
    if (v.trimmed().isEmpty())
        return v;
    switch (dataType()) {
    default:
        return v;
    case Float:
    case Int:
        break;
    }
    static const QStringList flt(QStringList() << "hex"
                                               << "lat"
                                               << "lon"
                                               << "mandala");
    if (flt.contains(u))
        return v;

    return QString("%1 %2").arg(v).arg(units());
}

QVariant FactData::_type_value(const QVariant &v) const
{
    if (v.isNull()) {
        switch (dataType()) {
        default:
            break;
        case Float:
            return QVariant::fromValue(0.0);
        case Int:
            return QVariant::fromValue(0);
        case Bool:
            return QVariant::fromValue(false);
        case Enum:
            return QVariant::fromValue(0);
        case Text:
            return QVariant::fromValue(QString());
        case Count:
            return size() > 0 ? QString::number(size()) : QString();
        }
    }
    return v;
}

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
    if ((treeType() != Group) && (!m_enumStrings.isEmpty())) {
        int ev = enumValue(v);
        if (ev >= 0)
            return enumText(ev);
        if (t == Enum)
            return v.toString();
    }
    if (t == Int) {
        if (units() == "mandala")
            return mandalaToString(static_cast<quint32>(v.toUInt()));
        return QString::number(v.toString().toLongLong());
    }
    if (t == Bool) {
        return QVariant(v.toBool()).toString();
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
    if (_check_type(v, QMetaType::Double) || _check_type(v, QMetaType::Float)) {
        double vf = v.toDouble();
        if (m_precision > 0) {
            double p = std::pow(10.0, m_precision);
            vf = cint(vf * p) / p;
        }
        QString s;
        if (vf == 0.0)
            s = "0";
        else {
            s = QString("%1").arg(vf, 0, 'f', m_precision > 0 ? m_precision : 8);
            if (s.contains('.')) {
                if (m_precision > 0)
                    s = s.left(s.indexOf('.') + 1 + m_precision);
                while (s.endsWith('0'))
                    s.chop(1);
                if (s.endsWith('.'))
                    s.chop(1);
            }
        }
        return s;
    }
    return v.toString();
}

bool FactData::isZero() const
{
    if (treeType() == Group) {
        for (auto f : facts()) {
            if (!f->isZero())
                return false;
        }
        return true;
    }

    const QVariant &v = value();
    if (v.isNull())
        return true;

    Flag t = dataType();
    if (t == Text)
        return v.toString().isEmpty();
    if (t == Float)
        return v.toDouble() == 0.0;
    const QString &s = v.toString().trimmed();
    if (s.isEmpty())
        return true;
    if (s == "0")
        return true;
    if (v.toInt() == 0)
        return true;

    return false;
}
bool FactData::isDefault() const
{
    if (treeType() == Group) {
        for (auto f : facts()) {
            if (!f->isDefault())
                return false;
        }
        return true;
    }

    if (m_defaultValue.isNull()) // never set
        return false;

    return value() == defaultValue();
}

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
void FactData::setPrecision(int v)
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
qreal FactData::increment(void) const
{
    return m_increment;
}
void FactData::setIncrement(qreal v)
{
    if (m_increment == v)
        return;
    m_increment = v;
    emit incrementChanged();
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
QString FactData::valueText()
{
    if (!m_valueTextUpdated) {
        m_valueTextUpdated = true;
        m_valueText = toText(value());
    }
    return m_valueText;
}
void FactData::setValueText(const QString &v)
{
    m_valueTextUpdated = true;
    if (m_valueText == v)
        return;
    m_valueText = v;
    emit valueTextChanged();
}
void FactData::updateValueText()
{
    m_valueTextUpdated = false;
    emit valueTextChanged();
}
QString FactData::text()
{
    if (!m_textUpdated) {
        m_textUpdated = true;
        m_text = _string_with_units(editorText());
    }
    return m_text;
}
QString FactData::editorText()
{
    QString v = valueText();
    switch (dataType()) {
    default:
        break;
    case Int:
        if (units() == "hex")
            v = QString::number(v.toUInt(), 16).toUpper();
        else if (units() == "s")
            v = AppRoot::timeToString(v.toUInt(), true);
        else if (units() == "min")
            v = AppRoot::timeToString(v.toUInt() * 60, false);
        else {
            bool ok;
            v.toFloat(&ok);
            if (!ok)
                break;
        }
        break;

    case Float: {
        bool ok;
        v.toFloat(&ok);
        if (!ok)
            break;
        break;
    }
    }
    return v;
}
void FactData::setText(const QString &v)
{
    m_textUpdated = true;
    if (m_text == v)
        return;
    m_text = v;
    emit textChanged();
}
void FactData::updateText()
{
    m_textUpdated = false;
    emit textChanged();
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
    return _type_value(m_defaultValue);
}
void FactData::setDefaultValue(const QVariant &v)
{
    if (m_defaultValue == v)
        return;
    m_defaultValue = v;
    emit defaultValueChanged();
}

QString FactData::mandalaToString(quint16 pid_raw) const
{
    Q_UNUSED(pid_raw)
    return QString();
}
quint16 FactData::stringToMandala(const QString &s) const
{
    Q_UNUSED(s)
    return 0;
}

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
        setModified(v != value());

    m_backupValid = true;

    if (m_backupValue == v)
        return;
    m_backupValue = v;
    emit backupValueChanged();
}
bool FactData::backupValid() const
{
    return m_backupValid;
}

void FactData::backup()
{
    for (auto i : facts()) {
        i->backup();
    }

    setModified(false);

    if (treeType() == Group)
        return;

    setBackupValue(value());
}
void FactData::restore()
{
    if (!modified())
        return;
    if (size()) {
        for (auto f : facts()) {
            f->restore();
        }
    }
    if (treeType() != Group && backupValid()) {
        setValue(backupValue());
    }
    setModified(false);
}
void FactData::restoreDefaults()
{
    for (auto i : facts()) {
        i->restoreDefaults();
    }

    if (m_defaultValue.isNull())
        return;

    setValue(defaultValue());
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
    const QVariant v = value();
    QString s = toText(v);
    bool rm = false;

    if (!m_defaultValue.isNull()) {
        rm = v == defaultValue() || s == defaultValue() || s == toText(defaultValue());
    }

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
