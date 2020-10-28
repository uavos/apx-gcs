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

#include <Fact/Fact.h>

class NodeScript : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title MEMBER _title);
    Q_PROPERTY(QString source MEMBER _source);

public:
    explicit NodeScript(Fact *fact);

    inline QString title() const { return _title; }
    inline QString source() const { return _source; }
    inline QByteArray code() const { return _code; }

    inline bool error() const { return _error; }
    inline QString log() const { return _log; }

    QMap<QString, QString> constants;

private:
    //compiler
    QProcess proc;
    QTemporaryFile srcFile{"XXXXXX.c"};
    QString outFileName;

    QString _title;
    QString _source;
    QByteArray _code;

    bool _error;
    QString _log;

    Fact *_fact;

    QString _value_s;

    bool _compile(QString src);

    bool _compile_pawn();
    bool _compile_wasm();

    void _updateFactText();

private slots:
    void factValueChanged();

public slots:
    void setSource(QString title, QString source);

signals:
    void compiled();
};
