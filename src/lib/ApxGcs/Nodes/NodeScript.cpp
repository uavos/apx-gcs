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
#include "NodeScript.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <App/AppRoot.h>
#include <Vehicles/Vehicles.h>

NodeScript::NodeScript(Fact *fact)
    : QObject(fact)
    , _fact(fact)
{
    srcFile.setFileTemplate("XXXXXX.cpp");
    srcFile.open();
    outFileName = srcFile.fileName() + ".code";
    proc.setProcessChannelMode(QProcess::MergedChannels);

    connect(fact, &Fact::valueChanged, this, &NodeScript::factValueChanged, Qt::QueuedConnection);

    _updateFactText();
}
void NodeScript::factValueChanged()
{
    QString value = _fact->value().toString();
    if (_value_s == value)
        return;

    QStringList st = value.split(',', Qt::KeepEmptyParts);
    QString title = st.value(0);
    QString src = st.value(1);
    _title = title;

    QByteArray src_ba = QByteArray::fromHex(src.toLocal8Bit());
    _source = qUncompress(src_ba);

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
    _updateFactText();
}

void NodeScript::_updateFactText()
{
    QString text;
    if (error())
        text = tr("error");
    else if (code().isEmpty())
        text = tr("empty");
    else {
        size_t size = code().size(); // + qCompress(source().toLocal8Bit(), 9).size();
        text = AppRoot::capacityToString(size, 2);
        if (!_title.isEmpty())
            text = QString("%1 (%2)").arg(_title).arg(text);
    }
    _fact->setText(text);
}

void NodeScript::setSource(QString title, QString source)
{
    //qDebug() << "set src:" << title;
    _title = title.simplified().trimmed();
    _value_s.clear();
    QStringList st;
    st << _title;
    st << qCompress(source.toLocal8Bit(), 9).toHex().toUpper();
    _fact->setValue(st.join(','));
}

bool NodeScript::_compile(QString src)
{
    qDebug() << "compiling:" << _title << src.size();
    _code.clear();
    _error = false;
    srcFile.resize(0);
    srcFile.flush();
    QFile::remove(outFileName);
    if (src.trimmed().isEmpty()) {
        emit compiled();
        return true;
    }
    //qDebug()<<src;
    QTextStream s(&srcFile);
    s << src;
    s.flush();
    srcFile.flush();

    //process
    _compile_wasm();

    bool rv = true;
    if (!proc.waitForFinished()) {
        apxMsgW() << "script compiler error:" << proc.errorString();
        rv = false;
    } else if (proc.exitCode() != 0)
        rv = false;
    _log = proc.isOpen() ? proc.readAll() : QString();
    if (!rv)
        _log.append("\n\n" + proc.errorString());

    //log file
    QTemporaryFile logFile;
    logFile.setFileTemplate(QDir::tempPath() + "/script_log");
    logFile.setAutoRemove(false);
    if (logFile.open()) {
        QTextStream s(&logFile);
        s << proc.program() + " " + proc.arguments().join(' ');
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

bool NodeScript::_compile_pawn()
{
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
    args << srcFile.fileName();
    proc.start(QCoreApplication::applicationDirPath() + "/pawncc", args);
    return true;
}

bool NodeScript::_compile_wasm()
{
    QString cc = "wasmcc";
    AppPlugin *p = App::plugin("ScriptCompiler");
    if (p) {
        QString pcc = p->control->property("cc").toString();
        if (!pcc.isEmpty() && QFile::exists(pcc))
            cc = pcc;
    }

    QStringList args;
    args << "--sysroot=" + AppDirs::res().absoluteFilePath("scripts/wasm/sysroot");
    args << "-O3";
    args << "-nostdlib";
    args << "-z"
         << "stack-size=8192";
    args << "-Wl,--initial-memory=65536";
    args << "-Wl,--export=main";
    args << "-o" + outFileName;
    args << srcFile.fileName();
    args << "-Wl,--export=__heap_base,--export=__data_end";
    args << "-Wl,--no-entry";
    args << "-Wl,--strip-all";
    args << "-Wl,--allow-undefined";

    //qDebug() << args;

    proc.start(cc, args);
    return true;
}
