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
#pragma once

#include "FactBase.h"

class FactData : public FactBase
{
    Q_OBJECT

    Q_PROPERTY(FactBase::Flag dataType READ dataType WRITE setDataType NOTIFY dataTypeChanged)

    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QVariant backupValue READ backupValue WRITE setBackupValue NOTIFY backupValueChanged)
    Q_PROPERTY(bool backupValid READ backupValid NOTIFY backupValidChanged)

    Q_PROPERTY(bool modified READ modified WRITE setModified NOTIFY modifiedChanged)

    Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
    Q_PROPERTY(QVariant min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(QVariant max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(qreal increment READ increment WRITE setIncrement NOTIFY incrementChanged)

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString descr READ descr WRITE setDescr NOTIFY descrChanged)

    Q_PROPERTY(QString valueText READ valueText WRITE setValueText NOTIFY valueTextChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

    Q_PROPERTY(const QStringList &enumStrings READ enumStrings WRITE setEnumStrings NOTIFY
                   enumStringsChanged)

    Q_PROPERTY(QString units READ units WRITE setUnits NOTIFY unitsChanged)

    Q_PROPERTY(
        QVariant defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged)
public:
    explicit FactData(QObject *parent,
                      const QString &name,
                      const QString &title,
                      const QString &descr,
                      FactBase::Flags flags);

    void copyValuesFrom(const FactData *item);

    int enumValue(const QVariant &v) const;
    QString enumText(int idx) const;

    virtual bool isZero() const;
    bool isDefault() const;

    //Mandala support - must override in derived classes
    // to collect dict ids from vehicle mandala
    // default impl searches for parent facts and
    // returns the first nonzero data found
public:
    virtual QString mandalaToString(quint16 pid_raw) const;
    virtual quint16 stringToMandala(const QString &s) const;

public slots:
    virtual void backup();
    virtual void restore();
    void restoreDefaults();

public slots:
    void savePresistentValue();
    void loadPresistentValue();

public:
    FactBase::Flag dataType() const;
    void setDataType(FactBase::Flag v);

    QVariant value(void) const;
    Q_INVOKABLE virtual bool setValue(const QVariant &v);

    QVariant backupValue(void) const;
    void setBackupValue(const QVariant &v);
    bool backupValid() const;

    bool modified() const;
    void setModified(const bool &v);

    int precision(void) const;
    void setPrecision(int v);
    QVariant min(void) const;
    void setMin(const QVariant &v);
    QVariant max(void) const;
    void setMax(const QVariant &v);
    qreal increment(void) const;
    void setIncrement(qreal v);

    QString title(void) const;
    virtual void setTitle(const QString &v);
    QString descr(void) const;
    virtual void setDescr(const QString &v);

    QString valueText();
    void setValueText(const QString &v);

    QString text();
    void setText(const QString &v);

    Q_INVOKABLE QString editorText();

    virtual const QStringList &enumStrings() const;
    void setEnumStrings(const QStringList &v);
    void setEnumStrings(const QMetaEnum &v);

    QString units() const;
    void setUnits(const QString &v);

    QVariant defaultValue(void) const;
    void setDefaultValue(const QVariant &v);

protected:
    bool updateValue(const QVariant &v);
    void updateValueText();
    void updateText();

    virtual QString toText(const QVariant &v) const;

    QString prefsGroup() const;

    static int _string_to_bool(QString s);
    static int _string_to_int(const QString &s);
    static bool _check_type(const QVariant &v, QMetaType::Type t);
    static bool _check_int(const QVariant &v);

    QVariant _type_value(const QVariant &v) const;
    QString _string_with_units(const QString &v) const;

protected:
    Flag m_dataType{NoFlags};

    QVariant m_value;
    QVariant m_backupValue;
    bool m_backupValid{false};

    bool m_modified{false};

    int m_precision{-1};
    QVariant m_min;
    QVariant m_max;
    qreal m_increment{};

    QString m_title;
    QString m_descr;

    QString m_valueText;
    bool m_valueTextUpdated{};

    QString m_text;
    bool m_textUpdated{};

    QStringList m_enumStrings;

    QString m_units;

    QVariant m_defaultValue;

private slots:
    void getPresistentValue();

    //---------------------------------------
    // PROPERTIES

signals:
    void dataTypeChanged();

    void valueChanged();
    void backupValueChanged();
    void backupValidChanged();

    void modifiedChanged();

    void precisionChanged();
    void minChanged();
    void maxChanged();
    void incrementChanged();

    void titleChanged();
    void descrChanged();

    void valueTextChanged();
    void textChanged();
    void enumStringsChanged();

    void unitsChanged();

    void defaultValueChanged();
};
