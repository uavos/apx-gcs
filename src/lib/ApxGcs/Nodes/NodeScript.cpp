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
#include <Fleet/Fleet.h>

NodeScript::NodeScript(Fact *fact)
    : QObject(fact)
    , _fact(fact)
{
    srcFile.setFileTemplate(
        QFileInfo(srcFile.fileTemplate()).absoluteDir().absoluteFilePath("script-XXXXXX.cpp"));
    srcFile.open();
    // qDebug() << srcFile.fileName();

    outFileName = QFileInfo(srcFile.fileName())
                      .absoluteDir()
                      .absoluteFilePath(QFileInfo(srcFile.fileName()).baseName() + ".wasm");
    proc.setProcessChannelMode(QProcess::MergedChannels);

    _plugin = App::plugin("ScriptCompiler");

    _update_cc_args();

    connect(fact, &Fact::valueChanged, this, &NodeScript::factValueChanged, Qt::QueuedConnection);

    _updateFactText();
}
void NodeScript::_update_cc_args()
{
    // find compiler
    cc = "wasmcc";
    if (_plugin) {
        cc.clear();
        auto c = _plugin->control;
        if (!c->property("enabled").toBool()) {
            // connect(c, SIGNAL("available"), this, SLOT("_update_cc_args"));
            return;
        }
        QString pcc = c->property("cc").toString();
        if (!pcc.isEmpty() && QFile::exists(pcc))
            cc = pcc;
    }

    if (cc.isEmpty())
        return;

    // parse compiler args - fill from json file
    cc_args.clear();
    QFile ftasks(AppDirs::res().filePath("scripts/.vscode/tasks.json"));
    if (ftasks.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(ftasks.readAll());
        ftasks.close();
        //qDebug() << json;
        for (const auto i : json["tasks"].toArray()) {
            const auto jso = i.toObject();
            if (!jso.value("group").toObject().value("isDefault").toBool())
                continue;
            QHash<QString, QString> map;
            map.insert("config:wasm.sysroot", AppDirs::scripts().absoluteFilePath("sysroot"));
            map.insert("config:wasm.flags", "-I" + AppDirs::scripts().absoluteFilePath("include"));
            map.insert("fileDirname", QFileInfo(srcFile.fileName()).absolutePath());
            map.insert("fileBasenameNoExtension", QFileInfo(srcFile.fileName()).baseName());
            map.insert("file", srcFile.fileName());
            for (auto i : jso.value("args").toArray()) {
                auto s = i.toVariant().toString();
                if (s.contains("${")) {
                    for (auto k : map.keys())
                        s.replace(QString("${%1}").arg(k), map.value(k));
                }
                cc_args.append(s);
            }
            break;
        }
    }
    //qDebug() << cc << cc_args;
}

void NodeScript::factValueChanged()
{
    QString value = _fact->value().toString();
    if (_value_s == value)
        return;

    _code.clear();
    _source.clear();
    _title.clear();
    _log.clear();
    _error = false;

    if (cc.isEmpty()) {
        _updateFactText();
        apxMsgW() << tr("Script compiler is missing");
        return;
    }

    //qDebug() << value;

    QStringList st = value.split(',', Qt::KeepEmptyParts);
    QString title = st.value(0);
    QString src = st.value(1);
    QString src_code = st.value(2);

    _title = title;

    if (!src.isEmpty()) {
        QByteArray src_ba = QByteArray::fromHex(src.toLocal8Bit());
        _source = qUncompress(src_ba);
        if (src_code.isEmpty())
            _compile(_source);
    }

    // compile if necessary

    if (src_code.isEmpty()) {
        QString code_string;
        if (!code().isEmpty())
            code_string = qCompress(code(), 9).toHex().toUpper();

        st.clear();
        st << _title;
        if (!code_string.isEmpty()) {
            st << src;
            st << code_string;
        }
        _value_s = st.join(',');

        //qDebug() << _value_s;

        _fact->setValue(_value_s);
    } else {
        QByteArray src_ba = QByteArray::fromHex(src_code.toLocal8Bit());
        _code = qUncompress(src_ba);
    }

    _updateFactText();
}

void NodeScript::_updateFactText()
{
    QString text;
    if (cc.isEmpty())
        text = "no compiler";
    else if (error())
        text = tr("error");
    else if (code().isEmpty())
        text = tr("empty");
    else {
        size_t size = code().size(); // + qCompress(source().toLocal8Bit(), 9).size();
        text = AppRoot::capacityToString(size, 2);
        if (!_title.isEmpty())
            text = QString("%1 (%2)").arg(_title).arg(text);
    }
    _fact->setValueText(text);
}

void NodeScript::setSource(QString title, QString source)
{
    //qDebug() << "set src:" << title;
    _title = title.simplified().trimmed();
    _title.remove(',');
    _value_s.clear();
    QStringList st;
    st << _title;
    st << qCompress(source.toLocal8Bit(), 9).toHex().toUpper();
    _fact->setValue(st.join(','));
}

bool NodeScript::_compile(QString src)
{
    if (cc.isEmpty())
        return true;

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
        qDebug() << "compiled:" << outFileName;
        QFile file(outFileName);
        if (file.open(QFile::ReadOnly)) {
            _code = file.readAll();
        }
    }
    _error = !rv;
    emit compiled();
    //qDebug() << "compile" << rv;
    return rv;
}

bool NodeScript::_compile_wasm()
{
    qDebug() << cc << cc_args;
    proc.start(cc, cc_args);
    return true;
}

bool NodeScript::saveToFile(QString fname)
{
    QFile file(fname);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot write file")
                  << QString("%1:\n%2.").arg(fname).arg(file.errorString());
        return false;
    }
    if (title() != QFileInfo(fname).baseName())
        setSource(QFileInfo(fname).baseName(), source());
    QTextStream s(&file);
    s << source();
    s.flush();
    file.close();
    _updateWatcher(fname);
    return true;
}
bool NodeScript::loadFromFile(QString fname)
{
    QFile file(fname);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot read file")
                  << QString("%1:\n%2.").arg(fname).arg(file.errorString());
        return false;
    }
    QTextStream s(&file);
    setSource(QFileInfo(fname).baseName(), s.readAll());
    file.close();
    _updateWatcher(fname);
    return true;
}
void NodeScript::_updateWatcher(QString fileName)
{
    if (_watcher) {
        if (_watcher->files().contains(fileName))
            return;
        _watcher->deleteLater();
    }
    _watcher = new QFileSystemWatcher(this);
    _watcher->addPath(fileName);
    connect(_watcher, &QFileSystemWatcher::fileChanged, this, &NodeScript::fileChanged);
}
void NodeScript::fileChanged(const QString &path)
{
    loadFromFile(path);
}
