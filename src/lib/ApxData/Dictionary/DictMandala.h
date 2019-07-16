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
#ifndef DictMandala_H
#define DictMandala_H
#include <QtCore>
class Mandala;
//=============================================================================
class DictMandala : private QObject
{
    Q_OBJECT
public:
    DictMandala(QObject *parent = nullptr);
    ~DictMandala();

    QHash<QString, QVariant> constants; // <name,value> enums in form varname_ENUM
    QHash<QString, quint16> special;    // <name,id>
    QStringList names;

    static QString hash();

    struct Entry
    {
        quint16 id;
        uint vtype;
        void *ptr;
        bool send_set; //use idx_set to send uplink
        QMetaType::Type type;
        QString name;
        QString descr;
        QString units;
        QStringList opts;
        Entry()
            : id(0)
            , vtype(0)
            , ptr(nullptr)
        {}
    };
    QList<Entry> items;
    QHash<quint16, int> idPos;

    double readValue(const Entry &i);
    QByteArray packValue(const Entry &i, double v);
    QByteArray packSetValue(const Entry &i, double v);
    QByteArray packVectorValue(const Entry &i, double v1, double v2, double v3);
    QByteArray packPointValue(const Entry &i, double v1, double v2);

    bool unpackValue(quint16 id, const QByteArray &data);
    bool unpackStream(const QByteArray &data);

private:
    Mandala *m;
    uint8_t tmp[32];

    bool isComplex(const Entry &i);

    void append(quint16 id,
                QMetaType::Type type,
                const QString &name,
                const QString &descr,
                const QString &units,
                const QStringList &opts = QStringList());
};
//=============================================================================
#endif
