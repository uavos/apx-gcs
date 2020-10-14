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
#include "ScriptCompiler.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Vehicles/Vehicles.h>

ScriptCompiler::ScriptCompiler(Fact *fact)
    : QObject(fact)
    , _fact(fact)
{
    tmpFile.open();
    outFileName = tmpFile.fileName() + "-compiled.amx";
    pawncc.setProgram(QCoreApplication::applicationDirPath() + "/pawncc");
    pawncc.setProcessChannelMode(QProcess::MergedChannels);

    connect(fact, &Fact::valueChanged, this, &ScriptCompiler::factValueChanged, Qt::QueuedConnection);
}
void ScriptCompiler::factValueChanged()
{
    QString value = _fact->value().toString();
    if (_value_s == value)
        return;

    QStringList st = value.split(',', Qt::KeepEmptyParts);
    QString title = st.value(0);
    QString src = st.value(1);
    _title = title;
    _source = qUncompress(QByteArray::fromHex(src.toLocal8Bit()));

    _compile(_source);

    QString code_string;
    if (!code().isEmpty())
        code_string = qCompress(code(), 9).toHex().toUpper();

    st.clear();
    st << _title;
    st << src;
    st << code_string;
    _value_s = st.join(',');

    _fact->setValue(_value_s);
}

void ScriptCompiler::setSource(QString title, QString source)
{
    //qDebug() << "set src:" << title;
    _title = title.simplified().trimmed();
    _value_s.clear();
    QStringList st;
    st << _title;
    st << qCompress(source.toLocal8Bit(), 9).toHex().toUpper();
    _fact->setValue(st.join(','));
}

bool ScriptCompiler::_compile(QString src)
{
    qDebug() << "compiling:" << _title << src.size();
    _code.clear();
    _error = false;
    tmpFile.resize(0);
    tmpFile.flush();
    QFile::remove(outFileName);
    if (src.trimmed().isEmpty()) {
        emit compiled();
        return true;
    }
    //qDebug()<<src;
    QTextStream s(&tmpFile);
    s << src;
    s.flush();
    tmpFile.flush();
    //process
    QStringList args;
    args << "-d0";
    args << "-O3";
    args << "-v2";
    args << "-r";
    //fill mandala constants
    if (constants.isEmpty()) {
        Vehicle *vehicle = Vehicles::instance()->f_local;
        if (vehicle) {
            for (auto f : vehicle->f_mandala->uid_map.values()) {
                QString s = f->mpath().replace('.', '_');
                constants.insert(s, QString::number(f->uid()));
            }
            for (auto s : vehicle->f_mandala->constants.keys()) {
                constants.insert(s, vehicle->f_mandala->constants.value(s).toString());
            }
        }
    }

    for (auto s : constants.keys()) {
        args << s + "=" + constants.value(s);
    }
    args << "-i" + AppDirs::res().absoluteFilePath("scripts/pawn/include");
    args << "-i" + AppDirs::scripts().absoluteFilePath("pawn");
    args << "-i" + AppDirs::scripts().absoluteFilePath(".");
    args << "-o" + outFileName;
    args << tmpFile.fileName();
    pawncc.setArguments(args);
    pawncc.start();
    bool rv = true;
    if (!pawncc.waitForFinished()) {
        apxMsgW() << "pawncc error:" << pawncc.errorString();
        rv = false;
    } else if (pawncc.exitCode() != 0)
        rv = false;
    _log = pawncc.isOpen() ? pawncc.readAll() : QString();
    if (!rv)
        _log.append("\n\n" + pawncc.errorString());

    //log file
    QTemporaryFile logFile;
    logFile.setFileTemplate(QDir::tempPath() + "/pawncc_log");
    logFile.setAutoRemove(false);
    if (logFile.open()) {
        QTextStream s(&logFile);
        s << pawncc.program() + " " + pawncc.arguments().join(' ');
        s << "\n\n";
        s << _log;
        s.flush();
        logFile.flush();
        logFile.close();
    }

    if (rv) {
        //read out data
        QFile file(outFileName);
        if (file.open(QFile::ReadOnly)) {
            _code = file.readAll();
        }
    }
    _error = !rv;
    emit compiled();
    //qDebug()<<"compile"<<rv<<m_outData.size();//<<getLog();
    return rv;
}
