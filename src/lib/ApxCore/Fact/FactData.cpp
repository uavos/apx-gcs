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
#include "FactData.h"
#include <App/AppRoot.h>
#include <cmath>
//=============================================================================
FactData::FactData(
    QObject *parent, const QString &name, const QString &title, const QString &descr, Flags flags)
    : FactBase(parent, name, flags)
    , backup_set(false)
    , bindedFactData(nullptr)
    , m_dataType(NoFlags)
    , m_modified(false)
    , m_precision(-1)
    , m_title()
    , m_descr()
{
    connect(this, &FactData::enumStringsChanged, this, &FactData::textChanged);

    connect(this, &FactData::dataTypeChanged, this, &FactData::defaults);
    connect(this, &FactData::dataTypeChanged, this, &FactData::valueChanged);
    connect(this, &FactData::dataTypeChanged, this, &FactData::textChanged);

    setTitle(title);
    setDescr(descr);

    setDataType(Flag(uint(flags) & DataMask));
}
//=============================================================================
bool FactData::vtype(const QVariant &v, QMetaType::Type t) const
{
    return static_cast<QMetaType::Type>(v.type()) == t;
}
//=============================================================================
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
    emit dataTypeChanged();
}
//=============================================================================
QVariant FactData::value(void) const
{
    if (bindedFactData)
        return bindedFactData->value();
    return m_value;
}
//=============================================================================
bool FactData::setValue(const QVariant &v)
{
    if (bindedFactData)
        return bindedFactData->setValue(v);
    //filter the same
    if (vtype(v, QMetaType::QByteArray)) {
        if (vtype(m_value, QMetaType::QByteArray) && m_value.toByteArray() == v.toByteArray())
            return false;
    } else if (m_value == v)
        return false;
    QVariant vx = v;
    int ev = enumValue(v);
    bool ok = false;
    if (treeType() != Group) {
        switch (dataType()) {
        case Enum:
            //qDebug()<<"set"<<path()<<vx<<"->"<<ev;
            if (ev < 0) {
                if (m_enumStrings.size() == 2) {
                    //try boolean strings
                    QString s = v.toString();
                    if (s == "true" || s == "on" || s == "yes")
                        ev = enumValue(1);
                    else if (s == "false" || s == "off" || s == "no")
                        ev = enumValue(0);
                    if (ev < 0) {
                        if (m_value.isNull())
                            ev = 0;
                        else
                            return false;
                    }
                } else if (m_value.isNull())
                    ev = 0;
                else
                    return false;
            }
            vx = ev;
            break;
        case Bool:
            if (ev < 0) {
                QString s = v.toString().toLower();
                vx = ((s == "true" || s == "1" || s == "on" || s == "yes") || v.toUInt() > 0)
                         ? true
                         : false;
            } else
                vx = ev;
            break;
        case Text:
            if (ev >= 0)
                vx = enumText(ev);
            break;
        case Int:
            if (ev >= 0)
                vx = ev;
            else if (m_enumStrings.size() > 1)
                return false;
            else if (!vtype(v, QMetaType::Int)) {
                QString s = v.toString();
                int i = 0;
                if (units() == "time" && s.contains(':')) {
                    i = AppRoot::timeFromString(s);
                    ok = true;
                }
                if (ok == false && units() == "hex") {
                    i = s.toInt(&ok, 16);
                }
                if (!ok) {
                    i = s.toInt(&ok);
                    if (!ok)
                        i = round(s.toDouble(&ok));
                    if (!ok)
                        i = s.toInt(&ok, 16);
                }
                if (!ok)
                    return false;
                if ((!m_min.isNull()) && i < m_min.toInt())
                    i = m_min.toInt();
                if ((!m_max.isNull()) && i > m_max.toInt())
                    i = m_max.toInt();
                vx = i;
            } else {
                int i = v.toInt();
                if ((!m_min.isNull()) && i < m_min.toInt())
                    i = m_min.toInt();
                if ((!m_max.isNull()) && i > m_max.toInt())
                    i = m_max.toInt();
                vx = i;
            }
            break;
        case Float:
            if (!vtype(v, QMetaType::Double)) {
                QString s = v.toString();
                if (units() == "lat") {
                    vx = AppRoot::latFromString(s);
                } else if (units() == "lon") {
                    vx = AppRoot::lonFromString(s);
                } else {
                    double d = s.toDouble(&ok);
                    if (!ok) {
                        //try boolean
                        if (s == "true" || s == "on" || s == "yes")
                            d = 1;
                        else if (s == "false" || s == "off" || s == "no")
                            d = 0;
                        else
                            return false;
                    }
                    if ((!m_min.isNull()) && d < m_min.toInt())
                        d = m_min.toInt();
                    if ((!m_max.isNull()) && d > m_max.toInt())
                        d = m_max.toInt();
                    vx = d;
                }
            } else
                vx = v.toDouble();
            break;
        case Mandala:
            vx = stringToMandala(v.toString());
            break;
        case Script:
            vx = v.toString();
            break;
        default:
            if (ev >= 0)
                vx = ev;
            break;
        }
    }
    if (m_value == vx)
        return false;
    if (dataType() == Float || dataType() == Text) {
        QVariant vbak = m_value;
        QString sv = text();
        m_value = vx;
        if (sv == text()) {
            m_value = vbak;
            return false;
        }
    } else {
        m_value = vx;
    }

    if (backup_set)
        setModified(backup_value != m_value);
    emit valueChanged();
    emit textChanged();
    return true;
}
//=============================================================================
int FactData::enumValue(const QVariant &v) const
{
    if (bindedFactData)
        return bindedFactData->enumValue(v);
    if (m_enumStrings.isEmpty())
        return -1;
    QString s = v.toString();
    int idx = -1;
    if (vtype(v, QMetaType::Int))
        idx = v.toInt();
    else if (vtype(v, QMetaType::UInt))
        idx = v.toUInt();
    else
        idx = m_enumStrings.indexOf(s);
    if (idx >= 0 && idx < m_enumStrings.size()) {
        if (idx < m_enumValues.size())
            idx = m_enumValues.at(idx);
    } else {
        bool ok = false;
        int i = s.toInt(&ok);
        if (ok) {
            if (!m_enumValues.isEmpty()) {
                if (m_enumValues.contains(i))
                    idx = i;
            } else if (i >= 0 && i < m_enumStrings.size())
                idx = i;
        }
    }
    return idx;
}
QString FactData::enumText(int v) const
{
    if (bindedFactData)
        return bindedFactData->enumText(v);
    if (!m_enumValues.isEmpty()) {
        if (m_enumValues.contains(v))
            return m_enumStrings.at(m_enumValues.indexOf(v));
    } else {
        if (v >= 0 && v < m_enumStrings.size())
            return m_enumStrings.at(v);
    }
    return QString();
}
//=============================================================================
bool FactData::isZero() const
{
    if (bindedFactData)
        return bindedFactData->isZero();
    if (treeType() == Group) {
        for (int i = 0; i < size(); i++) {
            FactData *f = child(i);
            if (!f->isZero())
                return false;
        }
        return true;
    }
    if (dataType() == Text)
        return text().isEmpty();
    if (dataType() == Script)
        return value().toString().isEmpty();
    const QString &s = text().trimmed();
    if (s.isEmpty())
        return true;
    /*if(dataType()==EnumData){
    return m_value.toInt()==0 && (s=="off"||s=="default"||s=="auto"||s==QVariant::fromValue(false).toString());
  }*/
    if (s == "0")
        return true;
    if (dataType() == Float)
        return m_value.toDouble() == 0.0;
    if (m_value.toInt() == 0)
        return true;
    if (m_value.isNull())
        return true;
    return false;
}
//=============================================================================
//=============================================================================
bool FactData::modified() const
{
    if (bindedFactData)
        return bindedFactData->modified();
    return m_modified;
}
void FactData::setModified(const bool &v, const bool &recursive)
{
    if (bindedFactData) {
        bindedFactData->setModified(v, recursive);
        return;
    }
    if (recursive) {
        for (int i = 0; i < size(); ++i) {
            child(i)->setModified(v, recursive);
        }
    }
    if (m_modified == v)
        return;
    m_modified = v;
    //qDebug() << path();
    emit modifiedChanged();
}
int FactData::precision(void) const
{
    if (bindedFactData)
        return bindedFactData->precision();
    return m_precision;
}
void FactData::setPrecision(const int &v)
{
    if (bindedFactData) {
        bindedFactData->setPrecision(v);
        return;
    }
    if (m_precision == v)
        return;
    m_precision = v;
    emit precisionChanged();
    emit textChanged();
}
QVariant FactData::min(void) const
{
    if (bindedFactData)
        return bindedFactData->min();
    return m_min;
}
void FactData::setMin(const QVariant &v)
{
    if (bindedFactData) {
        bindedFactData->setMin(v);
        return;
    }
    if (m_min == v)
        return;
    m_min = v;
    emit minChanged();
    if (v.isNull())
        return;
    if (value() < m_min)
        setValue(m_min);
}
QVariant FactData::max(void) const
{
    if (bindedFactData)
        return bindedFactData->max();
    return m_max;
}
void FactData::setMax(const QVariant &v)
{
    if (bindedFactData) {
        bindedFactData->setMax(v);
        return;
    }
    if (m_max == v)
        return;
    m_max = v;
    emit maxChanged();
    if (v.isNull())
        return;
    if (value() > m_max)
        setValue(m_max);
}
QString FactData::title(void) const
{
    if (bindedFactData && m_title.isEmpty())
        return bindedFactData->title();
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
    if (bindedFactData && m_descr.isEmpty())
        return bindedFactData->descr();
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
static double cint(double x)
{
    double i;
    if (std::modf(x, &i) >= .5)
        return x >= 0 ? std::ceil(x) : std::floor(x);
    else
        return x < 0 ? std::ceil(x) : std::floor(x);
}
QString FactData::text() const
{
    if (bindedFactData)
        return bindedFactData->text();
    const QVariant &v = value();
    if ((dataType() != Group) && (!m_enumStrings.isEmpty())) {
        int ev = enumValue(v);
        //qDebug()<<"text"<<v<<ev;
        if (ev >= 0 && ev < m_enumStrings.size())
            return enumText(ev);
        if (dataType() == Enum)
            return v.toString();
    }
    if (dataType() == Int) {
        if (units() == "hex")
            return QString::number(v.toUInt(), 16).toUpper();
        if (units() == "time") {
            return AppRoot::timeToString(v.toUInt(), true);
        }
        return QString::number(v.toInt());
    }
    if (dataType() == Mandala) {
        return mandalaToString(v.toUInt());
    }
    if (dataType() == Float) {
        if (units() == "lat") {
            return AppRoot::latToString(v.toDouble());
        }
        if (units() == "lon") {
            return AppRoot::lonToString(v.toDouble());
        }
    }
    if (vtype(v, QMetaType::QByteArray))
        return v.toByteArray().toHex().toUpper();
    if (vtype(v, QMetaType::Double)) {
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
const QStringList &FactData::enumStrings() const
{
    if (bindedFactData)
        return bindedFactData->enumStrings();
    return m_enumStrings;
}
const QList<int> &FactData::enumValues() const
{
    if (bindedFactData)
        return bindedFactData->enumValues();
    return m_enumValues;
}
void FactData::setEnumStrings(const QStringList &v, const QList<int> &enumValues)
{
    if (bindedFactData) {
        bindedFactData->setEnumStrings(v, enumValues);
        return;
    }
    if (m_enumStrings == v)
        return;
    m_enumStrings = v;
    if (v.size() == enumValues.size())
        m_enumValues = enumValues;
    else
        m_enumValues.clear();
    emit enumStringsChanged();
}
void FactData::setEnumStrings(const QMetaEnum &v)
{
    if (bindedFactData) {
        bindedFactData->setEnumStrings(v);
        return;
    }
    QStringList st;
    QList<int> vlist;
    for (int i = 0; i < v.keyCount(); ++i) {
        int vi = v.value(i);
        st.append(v.valueToKey(vi));
        vlist.append(vi);
    }
    setEnumStrings(st, vlist);
}
QString FactData::units() const
{
    if (bindedFactData)
        return bindedFactData->units();
    return m_units;
}
void FactData::setUnits(const QString &v)
{
    if (bindedFactData) {
        bindedFactData->setUnits(v);
        return;
    }
    if (m_units == v)
        return;
    m_units = v;
    emit unitsChanged();
}
QVariant FactData::defaultValue(void) const
{
    if (bindedFactData)
        return bindedFactData->defaultValue();
    return m_defaultValue;
}
void FactData::setDefaultValue(const QVariant &v)
{
    if (bindedFactData) {
        bindedFactData->setDefaultValue(v);
        return;
    }
    if (m_defaultValue == v)
        return;
    m_defaultValue = v;
    emit defaultValueChanged();
}
//=============================================================================
QString FactData::mandalaToString(quint16 mid) const
{
    //if (bindedFactData)
    //    return bindedFactData->mandalaToString(mid);
    QString s;
    for (const FactBase *i = parentFact(); i; i = i->parentFact()) {
        const FactData *f = static_cast<const FactData *>(i);
        s = f->mandalaToString(mid);
        if (!s.isEmpty())
            break;
    }
    return s;
}
quint16 FactData::stringToMandala(const QString &s) const
{
    //if (bindedFactData)
    //    return bindedFactData->stringToMandala(s);
    quint16 mid = 0;
    for (const FactBase *i = parentFact(); i; i = i->parentFact()) {
        const FactData *f = static_cast<const FactData *>(i);
        mid = f->stringToMandala(s);
        if (mid)
            break;
    }
    return mid;
}
const QStringList *FactData::mandalaNames() const
{
    //if (bindedFactData)
    //    return bindedFactData->mandalaNames();
    const QStringList *st = nullptr;
    for (const FactBase *i = parentFact(); i; i = i->parentFact()) {
        const FactData *f = static_cast<const FactData *>(i);
        st = f->mandalaNames();
        if (st)
            break;
    }
    return st;
}
//=============================================================================
void FactData::copyValuesFrom(const FactData *item)
{
    if (bindedFactData) {
        bindedFactData->copyValuesFrom(item);
        return;
    }
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
//=============================================================================
void FactData::backup()
{
    if (bindedFactData) {
        bindedFactData->backup();
        return;
    }
    if (size()) {
        for (int i = 0; i < size(); ++i) {
            child(i)->backup();
        }
        return;
    }
    if (treeType() == Group)
        return;
    backup_value = m_value;
    backup_set = true;
    setModified(false);
}
void FactData::restore()
{
    if (bindedFactData) {
        bindedFactData->restore();
        return;
    }
    if (!modified())
        return;
    if (size()) {
        for (int i = 0; i < size(); ++i) {
            child(i)->restore();
        }
    }
    if (treeType() != Group) {
        setValue(backup_value);
    }
    setModified(false);
}
void FactData::defaults()
{
    if (bindedFactData) {
        bindedFactData->defaults();
        return;
    }
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
        case Mandala:
            m_value = 0;
            break;
        default:
            m_value = QVariant();
        }
    }
}
//=============================================================================
void FactData::bind(FactData *fact)
{
    bool rebind = bindedFactData;
    if (bindedFactData) {
        disconnect(bindedFactData, nullptr, this, nullptr);
    }
    bindedFactData = fact;
    if (bindedFactData) {
        connect(fact, &FactData::valueChanged, this, &FactData::valueChanged);
        connect(fact, &FactData::textChanged, this, &FactData::textChanged);
        connect(fact, &FactData::enumStringsChanged, this, &FactData::enumStringsChanged);
        connect(fact, &FactData::dataTypeChanged, this, &FactData::dataTypeChanged);
        connect(fact, &FactData::modifiedChanged, this, &FactData::modifiedChanged);
        connect(fact, &FactData::precisionChanged, this, &FactData::precisionChanged);
        connect(fact, &FactData::minChanged, this, &FactData::minChanged);
        connect(fact, &FactData::maxChanged, this, &FactData::maxChanged);
        connect(fact, &FactData::titleChanged, this, &FactData::titleChanged);
        connect(fact, &FactData::descrChanged, this, &FactData::descrChanged);
        connect(fact, &FactData::unitsChanged, this, &FactData::unitsChanged);
        connect(fact, &FactData::defaultValueChanged, this, &FactData::defaultValueChanged);
        if (!dataType())
            setDataType(bindedFactData->dataType());
    }
    if (rebind) {
        emit valueChanged();
        emit textChanged();
        emit enumStringsChanged();
        emit dataTypeChanged();
        emit modifiedChanged();
        emit precisionChanged();
        emit minChanged();
        emit maxChanged();
        emit titleChanged();
        emit descrChanged();
        emit unitsChanged();
        emit defaultValueChanged();
    }
}
//=============================================================================
