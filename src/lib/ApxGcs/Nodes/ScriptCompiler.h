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
#pragma once

#include <Fact/Fact.h>

class ScriptCompiler : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title MEMBER _title);
    Q_PROPERTY(QString source MEMBER _source);

public:
    explicit ScriptCompiler(Fact *fact);

    inline QString title() const { return _title; }
    inline QString source() const { return _source; }
    inline QByteArray code() const { return _code; }

    inline bool error() const { return _error; }
    inline QString log() const { return _log; }

    QMap<QString, QString> constants;

private:
    //compiler
    QProcess pawncc;
    QTemporaryFile tmpFile;
    QString outFileName;

    QString _title;
    QString _source;
    QByteArray _code;

    bool _error;
    QString _log;

    Fact *_fact;

    QString _value_s;

    bool _compile(QString src);

    void _updateFactText();

private slots:
    void factValueChanged();

public slots:
    void setSource(QString title, QString source);

signals:
    void compiled();
};
