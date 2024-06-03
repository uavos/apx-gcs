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
#include "AppEngine.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <ApxMisc/FactQml.h>
#include <Fact/FactListModel.h>
#include <Fact/FactListModelActions.h>

AppEngine::AppEngine(QObject *parent)
    : QQmlApplicationEngine(parent)
{
    installExtensions(QJSEngine::AllExtensions);
    //installExtensions(QJSEngine::TranslationExtension|QJSEngine::ConsoleExtension);
    addImportPath(AppDirs::userPlugins().absolutePath());
    addImportPath("qrc:/");

    _jsTimer.setSingleShot(true);
    _jsTimer.setInterval(1);
    connect(&_jsTimer, &QTimer::timeout, this, &AppEngine::_queueExec);

    connect(this, &QQmlEngine::warnings, this, &AppEngine::warnings);

    // QML types register
    qmlRegisterType<FactQml>("APX.Facts", 1, 0, "Fact");
    qmlRegisterUncreatableType<FactListModel>("APX.Facts", 1, 0, "FactListModel", "Reference only");
    qmlRegisterUncreatableType<FactListModelActions>("APX.Facts",
                                                     1,
                                                     0,
                                                     "FactListModelActions",
                                                     "Reference only");

    qRegisterMetaType<QAbstractItemModel *>("QAbstractItemModel*");

    // script include file (default)
    QFile jsFile(AppDirs::res().filePath("scripts/gcs.js"));
    if (jsFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&jsFile);
        QString contents = stream.readAll();
        jsFile.close();
        jsexec(contents);
    } else {
        apxConsoleW() << "Resource not found:" << jsFile.fileName();
    }
    // script include file (user commands)
    QFile jsFile2(AppDirs::scripts().filePath("gcs.js"));
    if (jsFile2.open(QIODevice::ReadOnly)) {
        QTextStream stream(&jsFile2);
        QString contents = stream.readAll();
        jsFile2.close();
        jsexec(contents);
    }

    //add is queued to wait inherited constructors
    //connect(this,&Fact::itemAdded,this,&AppEngine::jsAddItem);//,Qt::QueuedConnection);
    //connect(this,&Fact::itemRemoved,this,&AppEngine::jsRemoveItem);//,Qt::QueuedConnection);
}
AppEngine::~AppEngine()
{
    qDebug() << "enigne destroyed";
}

void AppEngine::jsSyncObject(QObject *obj)
{
    jsSetProperty(globalObject(), obj->objectName(), jsCreateObject(obj));
    jsProtectPropertyWrite(obj->objectName());
}
QJSValue AppEngine::jsCreateObject(QObject *obj)
{
    QQmlEngine::setObjectOwnership(obj, QQmlEngine::CppOwnership);
    // Fact *f = qobject_cast<Fact *>(obj);
    // if (f)
    //     return f->jsobject();
    return newQObject(obj);
}

void AppEngine::jsSync(Fact *fact)
{
    if (fact->jsname().isEmpty())
        return;

    QJSValue v = globalObject();
    //qDebug() << fact << list;
    //build tree from root
    const FactList &list = fact->pathList();
    for (int i = list.size() - 1; i > 0; --i) {
        Fact *f = list.at(i);
        QJSValue vp = v.property(f->jsname());
        if (vp.isUndefined() || (!vp.isQObject()) || vp.toQObject() != f) {
            jsSetProperty(v, f->jsname(), jsCreateObject(f));
            jsProtectPropertyWrite(f->jspath());
        }
        v = vp;
    }
    //set the property and value
    jsSync(fact, v);
}

QJSValue AppEngine::jsSync(Fact *fact, QJSValue parent) //recursive
{
    //qDebug() << fact->path();
    QJSValue js_fact = jsCreateObject(fact);

    jsSetProperty(parent, fact->jsname(), js_fact);
    jsProtectPropertyWrite(fact->jspath());

    //sync children
    for (int i = 0; i < fact->size(); ++i)
        jsSync(fact->child(i), js_fact);
    //sync actions
    if (!fact->actions().isEmpty()) {
        for (auto i : fact->actions()) {
            jsSync(i, js_fact);
        }
    }
    return js_fact;
}

void AppEngine::jsSetProperty(QJSValue parent, QString name, QJSValue value)
{
    if (name.isEmpty() || name.contains('#'))
        return;

    if (value.isUndefined() || value.isNull()) {
        // delete property
        parent.deleteProperty(name);
        return;
    }

    QJSValue p = parent.property(name);
    if (!p.isUndefined()) {
        // modify already existing property
        if (p.strictlyEquals(value))
            return;
        qWarning() << "Rewriting property:" << name << value.toString() << p.toString()
                   << "for parent" << parent.toString();
        parent.deleteProperty(name);
    }

    parent.setProperty(name, value);
}

void AppEngine::jsProtectPropertyWrite(const QString path)
{
    if (path.isEmpty() || path.contains('#'))
        return;

    QString scope, name;
    if (path.contains('.')) {
        scope = path.left(path.lastIndexOf('.'));
        name = path.mid(scope.size() + 1);
    } else {
        scope = "this";
        name = path;
    }
    QString s = QString("Object.defineProperty(%1,'%2',{enumerable:false,configurable:"
                        "false,writable:false})")
                    .arg(scope)
                    .arg(name);
    // QString s = QString("%1.__defineSetter__('%2',function(v){print('error: read-only (%3)')})")
    //                 .arg(scope)
    //                 .arg(name)
    //                 .arg(path);
    s = QString("try{%1}catch(e){}").arg(s);
    jsexec_queued(s);
}

void AppEngine::jsProtectObjects(const QString path)
{
    _protectObjects(path, jsGetProperty(path));
}
void AppEngine::_protectObjects(const QString path, QJSValue obj)
{
    QJSValueIterator it(obj);
    while (it.hasNext()) {
        it.next();
        _protectObjects(path + '.' + it.name(), it.value());
    }
    if (!obj.isObject())
        return;

    QString s = QString("Object.seal(%1)").arg(path);

    s = QString("try{%1}catch(e){}").arg(s);
    jsexec_queued(s);
}

QJSValue AppEngine::jsexec(const QString s)
{
    QJSValue result;
    result = evaluate(s);
    if (result.isError()) {
        apxMsgW() << s << result.toString();
    }
    //qDebug() << result.toString();
    return result;
}
void AppEngine::jsexec_queued(const QString s)
{
    if (s.isEmpty() || _jsQueue.contains(s))
        return;
    _jsQueue.enqueue(s);
    _jsTimer.start();
}
void AppEngine::_queueExec()
{
    if (_jsQueue.isEmpty())
        return;
    QString s = _jsQueue.dequeue();
    jsexec(s);
    _jsTimer.start();
}

void AppEngine::sleep(quint16 ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec();
}

QByteArray AppEngine::jsToArray(QJSValue data)
{
    //qDebug()<<portID<<data.toString()<<data.isArray()<<data.toVariant().toByteArray().toHex();
    QByteArray ba;
    if (data.isString() || data.toString().contains(',')) {
        bool ok = false;
        foreach (QString sv, data.toString().trimmed().toLower().split(',', Qt::SkipEmptyParts)) {
            uint v;
            sv = sv.trimmed();
            if (sv.startsWith("0x"))
                v = sv.mid(2).toUInt(&ok, 16);
            else
                v = sv.toUInt(&ok, 10);
            if (!ok)
                break;
            ba.append(static_cast<char>(v));
        }
        if (!ok)
            return QByteArray();
    } else if (data.isNumber()) {
        ba.append(static_cast<char>(data.toInt()));
    } else
        return QByteArray();
    return ba;
}

QJSValue AppEngine::jsGetProperty(const QString path)
{
    QJSValue v = globalObject();
    for (auto i : path.split('.', Qt::SkipEmptyParts)) {
        QJSValue vp = v.property(i);
        if (vp.isUndefined())
            return QJSValue();
        v = vp;
    }
    return v;
}

QObject *AppEngine::loadQml(const QString qmlFile, const QVariantMap &opts)
{
    QString schk = qmlFile;
    if (schk.startsWith("qrc:"))
        schk.remove(0, 3);
    if (!QFile::exists(schk)) {
        apxMsgW() << tr("Not found").append(':') << schk;
        return nullptr;
    }

    QQmlComponent c(this, qmlFile, QQmlComponent::PreferSynchronous);
    do {
        if (c.isError())
            break;

        QObject *obj = c.beginCreate(rootContext());
        if (c.isError())
            break;

        for (auto key : opts.keys()) {
            QQmlProperty::write(obj, key.toUtf8(), opts.value(key));
        }

        if (c.isError())
            break;
        c.completeCreate();
        if (c.isError())
            break;

        return obj;
    } while (0);
    apxMsgW() << c.errorString();
    return nullptr;
}

void AppEngine::warnings(const QList<QQmlError> &warnings)
{
    for (auto w : warnings) {
        apxMsgW() << w;
    }
}
