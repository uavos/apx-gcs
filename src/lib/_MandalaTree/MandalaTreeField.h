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
#ifndef MandalaTreeField_H
#define MandalaTreeField_H
//=============================================================================
#include <QtCore>
#include "MandalaTree.h"
//=============================================================================
class MandalaTreeField : public MandalaTree
{
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "value")
public:
    explicit MandalaTreeField(MandalaTree *parent,
                              _mandala_index index,
                              QString name,
                              QString descr,
                              _mandala_field::_mf_type dtype,
                              QStringList opts,
                              QStringList optsDescr,
                              QString alias);

    //PROPERTIES
public:
    Q_PROPERTY(QString alias READ alias NOTIFY structChanged)
    Q_PROPERTY(QStringList opts READ opts NOTIFY structChanged)
    Q_PROPERTY(QStringList optsDescr READ optsDescr NOTIFY structChanged)
    Q_PROPERTY(quint8 dtype READ dtype NOTIFY structChanged)

    Q_PROPERTY(QString units READ units NOTIFY structChanged)
    Q_PROPERTY(QString caption READ caption NOTIFY structChanged) //descr without units
    Q_PROPERTY(int precision READ precision NOTIFY structChanged)

public:
    QVariant value(void) const;
    Q_INVOKABLE bool setValue(const QVariant &v);

    const QString &alias(void) const;
    const QStringList &opts(void) const;
    const QStringList &optsDescr(void) const;
    quint8 dtype(void) const;

    bool used(void) const;
    void setUsed(bool v);

    QString valueText(void) const;

    const QString &units() const;
    const QString &caption() const;
    int precision() const;

    //override
    QString descr(void) const;

private:
    QVariant m_value;

    _mandala_field::_mf_type m_dtype;

    QString m_alias;
    QStringList m_opts;
    QStringList m_optsDescr;

    bool m_used;

    QString m_units;
    QString m_caption;
    int m_precision;

signals:
    void structChanged(MandalaTree *item);
};
//Q_DECLARE_METATYPE(MandalaTreeField*)
//=============================================================================
#endif
