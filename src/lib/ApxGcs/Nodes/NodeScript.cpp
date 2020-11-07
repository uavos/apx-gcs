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
    srcFile.setFileTemplate(srcFile.fileTemplate() + ".cpp");
    srcFile.open();
    outFileName = QFileInfo(srcFile.fileName())
                      .absoluteDir()
                      .absoluteFilePath(QFileInfo(srcFile.fileName()).baseName() + ".wasm");
    proc.setProcessChannelMode(QProcess::MergedChannels);

    _update_cc_args();

    connect(fact, &Fact::valueChanged, this, &NodeScript::factValueChanged, Qt::QueuedConnection);

    _updateFactText();
}
void NodeScript::_update_cc_args()
{
    cc_args.clear();
    QFile ftasks(AppDirs::res().filePath("scripts/.vscode/tasks.json"));
    if (ftasks.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(ftasks.readAll());
        ftasks.close();
        //qDebug() << json;
        foreach (QJsonValue v, json["tasks"].toArray()) {
            if (!v["group"]["isDefault"].toBool())
                continue;
            QHash<QString, QString> map;
            map.insert("config:wasm.sysroot", AppDirs::scripts().absoluteFilePath("sysroot"));
            map.insert("config:wasm.flags", "-I" + AppDirs::scripts().absoluteFilePath("include"));
            map.insert("fileDirname", QFileInfo(srcFile.fileName()).absolutePath());
            map.insert("fileBasenameNoExtension", QFileInfo(srcFile.fileName()).baseName());
            map.insert("file", srcFile.fileName());
            for (auto a : v["args"].toArray().toVariantList()) {
                QString s = a.toString();
                if (s.contains("${")) {
                    for (auto k : map.keys())
                        s.replace(QString("${%1}").arg(k), map.value(k));
                }
                cc_args.append(s);
            }
            break;
        }
    }
    //qDebug() << cc_args;
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

bool NodeScript::_compile_wasm()
{
    QString cc = "wasmcc";
    AppPlugin *p = App::plugin("ScriptCompiler");
    if (p) {
        QString pcc = p->control->property("cc").toString();
        if (!pcc.isEmpty() && QFile::exists(pcc))
            cc = pcc;
    }

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
