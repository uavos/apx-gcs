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
#include "DictNode.h"
//=============================================================================
DictNode::DictNode(QObject *parent)
    : QObject(parent)
{}
//=============================================================================
//=============================================================================
DictNode::Info::Info()
    : valid(false)
{}
//=============================================================================
DictNode::DictInfo::DictInfo()
    : valid(false)
    , paramsCount(0)
{}
//=============================================================================
DictNode::Field::Field()
    : valid(false)
    , id(0)
    , ftype(-1)
    , array(0)
    , type(Void)
{}
void DictNode::Field::expandStrings()
{
    //extract opts from descr if any
    if (opts.size() == 0 && descr.contains('(')) {
        QString s(descr.mid(descr.indexOf('(') + 1).trimmed());
        opts = QStringList(s.left(s.lastIndexOf(')')).split(','));
        if (opts.size() < 2)
            opts.clear();
        else
            descr = descr.left(descr.indexOf('(')).trimmed();
    }
    if (opts.size() == 1)
        opts.clear();
    //truncate opts by '_'
    if (opts.size() && opts.first().contains("_")) {
        QStringList nopts;
        foreach (QString opt, opts)
            nopts.append(opt.mid(opt.lastIndexOf('_') + 1));
        opts = nopts;
    }
    //default opts
    if ((type == DictNode::Option) && (opts.isEmpty()))
        opts = QStringList() << "no"
                             << "yes";

    //trim opts
    for (auto i = 0; i < opts.size(); ++i) {
        opts.replace(i, opts.at(i).trimmed());
    }

    //array from name
    array = 0;
    if (name.contains("[")) {
        array = name.section("[", 1, 1).section("]", 0, 0).toInt();
        name = name.left(name.indexOf('['));
    }
    //trim title
    title = name;
    if (title.contains("_")) {
        title.remove(0, title.indexOf('_') + 1);
    }
    //units from descr
    if (descr.contains('[')) {
        QString s = descr.mid(descr.lastIndexOf('[') + 1);
        descr = descr.left(descr.indexOf('['));
        s = s.left(s.indexOf(']'));
        if (!s.contains(".."))
            units = s;
    }
    //groups from descr
    while (descr.contains(':')) {
        QString s = descr.left(descr.indexOf(':')).trimmed();
        groups.append(s);
        descr = descr.remove(0, descr.indexOf(':') + 1).trimmed();
        if (title.contains('_') && title.left(title.indexOf('_')).toLower() == s.toLower())
            title.remove(0, title.indexOf('_') + 1);
    }
}
//=============================================================================
DictNode::Dict::Dict()
    : commandsValid(false)
    , fieldsValid(false)
    , dataValid(false)
    , cached(false)
{}
void DictNode::Dict::reset(const QString &chash, int paramsCount)
{
    commandsValid = false;
    fieldsValid = false;
    cached = false;
    commands.clear();
    fields.clear();
    this->chash = chash;
    for (int i = 0; i < paramsCount; ++i) {
        Field f;
        fields.append(f);
    }
}
//=============================================================================
//=============================================================================
//=============================================================================
