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
#ifndef FactData_H
#define FactData_H
//=============================================================================
#include "FactBase.h"
//=============================================================================
class FactData : public FactBase
{
    Q_OBJECT

    Q_PROPERTY(FactBase::Flag dataType READ dataType WRITE setDataType NOTIFY dataTypeChanged)

    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

    Q_PROPERTY(bool modified READ modified WRITE setModified NOTIFY modifiedChanged)

    Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
    Q_PROPERTY(QVariant min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(QVariant max READ max WRITE setMax NOTIFY maxChanged)

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString descr READ descr WRITE setDescr NOTIFY descrChanged)

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
    QString enumText(int v) const;

    virtual bool isZero() const;

    void defaults();

    virtual void bind(FactData *fact);

    //Mandala support - must override in derived classes
    // to collect dict ids from vehicle mandala
    // default impl searches for parent facts and
    // returns the first nonzero data found
    virtual QString mandalaToString(quint16 mid) const;
    virtual quint16 stringToMandala(const QString &s) const;

private:
    FactData *child(int n) const { return qobject_cast<FactData *>(FactBase::child(n)); }
    FactData *child(const QString &name) const
    {
        return qobject_cast<FactData *>(FactBase::child(name));
    }

signals:
    //void childValueChanged(void);

public slots:
    virtual void backup();
    virtual void restore();

protected:
    QVariant backup_value;
    bool backup_set;
    QPointer<FactData> bindedFactData;

    bool vtype(const QVariant &v, QMetaType::Type t) const;

    bool updateValue(const QVariant &v);

    QString toText(const QVariant &v) const;

    QString prefsGroup() const;

private slots:
    void getPresistentValue();
public slots:
    void savePresistentValue();
    void loadPresistentValue();

    //---------------------------------------
    // PROPERTIES
public:
    FactBase::Flag dataType() const;
    void setDataType(FactBase::Flag v);

    QVariant value(void) const;
    Q_INVOKABLE virtual bool setValue(const QVariant &v);

    bool modified() const;
    virtual void setModified(const bool &v, const bool &recursive = false);

    int precision(void) const;
    void setPrecision(const int &v);
    QVariant min(void) const;
    void setMin(const QVariant &v);
    QVariant max(void) const;
    void setMax(const QVariant &v);

    QString title(void) const;
    void setTitle(const QString &v);
    QString descr(void) const;
    void setDescr(const QString &v);

    QString text() const;

    virtual const QStringList &enumStrings() const;
    virtual const QList<int> &enumValues() const;
    void setEnumStrings(const QStringList &v, const QList<int> &enumValues = QList<int>());
    void setEnumStrings(const QMetaEnum &v);

    QString units() const;
    void setUnits(const QString &v);

    QVariant defaultValue(void) const;
    void setDefaultValue(const QVariant &v);

protected:
    Flag m_dataType;

    QVariant m_value;

    bool m_modified;

    int m_precision;
    QVariant m_min;
    QVariant m_max;

    QString m_title;
    QString m_descr;

    QStringList m_enumStrings;
    QList<int> m_enumValues;

    QString m_units;

    QVariant m_defaultValue;

signals:
    void dataTypeChanged();

    void valueChanged();

    void modifiedChanged();

    void precisionChanged();
    void minChanged();
    void maxChanged();

    void titleChanged();
    void descrChanged();

    void textChanged();
    void enumStringsChanged();

    void unitsChanged();

    void defaultValueChanged();
};
//=============================================================================
#endif
