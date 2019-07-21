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
#ifndef DictNode_H
#define DictNode_H
#include <QtCore>
//=============================================================================
class DictNode : private QObject
{
    Q_OBJECT
public:
    DictNode(QObject *parent = nullptr);

    //data types
    enum DataType {
        Void = -1,
        //basic types
        Float,
        Byte,
        UInt,
        Option,
        String,
        StringL,
        MandalaID,
        //expanded
        Vector,
        Script,
        Hash,
        Array,
    };
    Q_ENUM(DataType)
    static inline QString dataTypeToString(int type)
    {
        return QMetaEnum::fromType<DataType>().valueToKey(type);
    }
    static inline DataType dataTypeFromString(const QString &s)
    {
        return static_cast<DataType>(QMetaEnum::fromType<DataType>().keyToValue(s.toUtf8()));
    }

    //general reply structures
    struct Info
    {
        bool valid;
        QString name;
        QString version;
        QString hardware;
        bool reconf;
        bool fwSupport;
        bool fwUpdating;
        bool addressing;
        bool rebooting;
        bool busy;
        Info();
    };
    struct DictInfo
    {
        bool valid;
        int paramsCount;
        QString chash;
        DictInfo();
    };

    //dictionary tree
    struct Command
    {
        uint cmd;
        QString name;
        QString descr;
    };
    struct Field
    {
        //used by protocol internally
        bool valid;
        quint16 id;
        int ftype;
        int array;
        QVariant value;
        //processed data
        DataType type;
        QString name;
        QString title;
        QString descr;
        QString units;
        QStringList opts;
        QStringList groups;
        QList<Field> subFields;
        Field();
        void expandStrings();
    };
    struct Dict
    {
        bool commandsValid;
        bool fieldsValid;
        bool dataValid;
        bool cached;
        QString chash;
        QList<Command> commands;
        QList<Field> fields;

        Dict();
        void reset(const QString &chash = QString(), int paramsCount = 0);
    };

    //node stats reply
    struct Stats
    {
        qreal vbat;
        qreal ibat;
        quint8 errCnt;
        quint8 canRxc;
        quint8 canAdr;
        quint8 canErr;
        quint8 cpuLoad;
        QByteArray dump;
    };

    //current state
    Info info;
    DictInfo dictInfo;
    Dict dict;
};
//=============================================================================
Q_DECLARE_METATYPE(DictNode::Info)
Q_DECLARE_METATYPE(DictNode::DictInfo)
Q_DECLARE_METATYPE(DictNode::Dict)
//=============================================================================
#endif
