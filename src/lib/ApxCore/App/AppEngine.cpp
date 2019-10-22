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
#include "AppEngine.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <ApxMisc/FactQml.h>
//=============================================================================
AppEngine::AppEngine(QObject *parent)
    : QQmlApplicationEngine(parent)
{
    installExtensions(QJSEngine::AllExtensions);
    //installExtensions(QJSEngine::TranslationExtension|QJSEngine::ConsoleExtension);
    addImportPath(AppDirs::userPlugins().absolutePath());
    addImportPath("qrc:/");

    // QML types register
    qmlRegisterType<FactQml>("APX.Facts", 1, 0, "Fact");
    qmlRegisterUncreatableType<FactListModel>("APX.Facts",
                                              1,
                                              0,
                                              "FactListModelActions",
                                              "Reference only");
    qmlRegisterUncreatableType<FactListModelActions>("APX.Facts",
                                                     1,
                                                     0,
                                                     "FactListModelActions",
                                                     "Reference only");

    jsRegisterFunctions();

    // script include file (default)
    QFile jsFile(AppDirs::res().filePath("scripts/gcs.js"));
    if (jsFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&jsFile);
        QString contents = stream.readAll();
        jsFile.close();
        jsexec(contents);
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
//=============================================================================
void AppEngine::jsSyncObject(QObject *obj)
{
    QQmlEngine::setObjectOwnership(obj, QQmlEngine::CppOwnership);
    jsSetProperty(globalObject(), obj->objectName(), newQObject(obj));
}
//=============================================================================
void AppEngine::jsSync(Fact *fact)
{
    if (fact->name().isEmpty())
        return;

    QList<FactBase *> list = fact->pathList();
    QJSValue v = globalObject();
    //qDebug() << fact << list;
    //build tree from root
    for (int i = list.size() - 1; i > 0; --i) {
        Fact *f = static_cast<Fact *>(list.at(i));
        QJSValue vp = v.property(f->name());
        if (vp.isUndefined() || (!vp.isQObject()) || vp.toQObject() != f) {
            jsSetProperty(v, f->name(), newQObject(f));
        }
        v = vp;
    }
    //set the property and value
    jsSync(fact, v);
    //collectGarbage();
}
//=============================================================================
QJSValue AppEngine::jsSync(Fact *fact, QJSValue parent) //recursive
{
    //qDebug() << fact->path();
    if (fact->name().isEmpty())
        return QJSValue();
    QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
    QJSValue js_fact = newQObject(fact);

    jsSetProperty(parent, fact->name(), js_fact);

    //sync children
    for (int i = 0; i < fact->size(); ++i)
        jsSync(fact->child(i), js_fact);
    //sync actions
    if (!fact->actions().isEmpty()) {
        //QJSValue js_actions = newObject();
        foreach (FactBase *i, fact->actions()) {
            Fact *f = static_cast<Fact *>(i);
            jsSync(f, js_fact);
            /*QQmlEngine::setObjectOwnership(f, QQmlEngine::CppOwnership);
            jsSetProperty(js_actions, f->name(), newQObject(f));
            if (f->bind() && f->bind()->parentFact() == nullptr) {
                //action opens fact page with no parent
                jsSync(f->bind(), js_fact);
            }*/
        }
        //js_fact.setProperty("action", js_actions);
    }
    return js_fact;
}
//=============================================================================
void AppEngine::jsSetProperty(QJSValue parent, const QString &name, QJSValue v)
{
    if ((!v.isUndefined()) && parent.hasProperty(name)) {
        QJSValue vp = parent.property(name);
        while (!vp.isUndefined()) {
            if (vp.isQObject() && vp.toQObject() == v.toQObject())
                return;
            //qWarning() << "Rewriting property:" << name << v.toString() << vp.toString()
            //           << "for parent" << parent.toString();
            break;
        }
    }
    parent.setProperty(name, v);
}
//=============================================================================
QJSValue AppEngine::jsexec(const QString &s)
{
    QJSValue result;
    result = evaluate(s);
    if (result.isError()) {
        apxMsgW() << result.toString();
    }
    return result;
}
//=============================================================================
void AppEngine::jsRegister(QString fname, QString description, QString body)
{
    jsexec(QString("function %1 { %2;};").arg(fname).arg(body));
    jsexec(QString("%1.info=\"%2\";").arg(fname.left(fname.indexOf('('))).arg(description));
    js_descr[fname] = description;
}
//=============================================================================
//=============================================================================
void AppEngine::jsRegisterFunctions()
{
    //system functions and objects
    jsRegister("help()", QObject::tr("print commands help"), "application.engine.help();");
    jsRegister("req(n)",
               QObject::tr("request var n from UAV"),
               "apx.vehicles.current.mandala[n].request();");
    jsRegister("send(n)",
               QObject::tr("send var n to UAV"),
               "apx.vehicles.current.mandala[n].send();");
    jsRegister("nodes()", QObject::tr("rescan bus nodes"), "apx.vehicles.current.nodes.request();");
    jsRegister("nstat()",
               QObject::tr("print nodes status"),
               "print('nodes statistics:');apx.vehicles.current.nodes.nstat();");
    jsRegister("serial(p,v)",
               QObject::tr("send data v to serial port ID p"),
               "apx.vehicles.current.sendSerial(p,v);");
    jsRegister("vmexec(f)",
               QObject::tr("execute function on VMs"),
               "apx.vehicles.current.vmexec(f);");
    jsRegister("sleep(n)", QObject::tr("sleep n milliseconds"), "application.engine.sleep(n);");
    jsRegister("next()", QObject::tr("switch to next vehicle"), "apx.vehicles.selectNext();");
    jsRegister("prev()", QObject::tr("switch to previous vehicle"), "apx.vehicles.selectPrev();");

    //some helper functions
    jsRegister("trigger(v,a,b)",
               QObject::tr("trigger value of v to a or b"),
               "if(v==a)return b; else return a;");
    jsRegister("bound(v)",
               QObject::tr("wrap angle -180..+180"),
               "while(v>=180)v-=360;while(v<-180)v+=360;return v;");
    jsRegister("ls(a,b)",
               QObject::tr("print members of type b for scope a"),
               "for(var i in a)if(typeof(a[i])==b || !b)print(i+\" - \"+typeof(a[i]));");
    jsRegister("vars(a)",
               QObject::tr("print variables for scope a"),
               "if(arguments.length==0)a=this;for(var i in "
               "a)if(typeof(a[i])=='number')print(i+\"=\"+a[i]);");
    jsRegister(
        "func(a)",
        QObject::tr("print functions for scope a"),
        "if(arguments.length==0)a=this;for(var i in a)if(typeof(a[i])=='function')print(i);");
    //predefined commands
    jsRegister("ahrs()", QObject::tr("reset AHRS"), "req('roll');");
    jsRegister("zrc()",
               QObject::tr("reset pilot controls"),
               "rc_roll=0;rc_pitch=0;rc_throttle=0;rc_yaw=0;");
    jsRegister("zps()", QObject::tr("reset barometric altitude on ground"), "altps_gnd=0;");
    jsRegister("ned()",
               QObject::tr("reset local GPS coordinates"),
               "home_lat=gps_lat;home_lon=gps_lon;home_hmsl=gps_hmsl;");
    jsRegister("hmsl()", QObject::tr("reset local GPS altitude"), "home_hmsl=gps_hmsl;");
}
//=============================================================================
void AppEngine::help()
{
    QString s;
    s += "<html><table>";
    foreach (const QString &cmd, js_descr.keys()) {
        s += "<tr><td valign='middle'>";
        s += "<nobr>&nbsp;<font color=cyan>";
        s += cmd;
        s += "</font></nobr></td><td>"; // width='100%'>";
        s += "<font color=gray> &nbsp;" + js_descr.value(cmd) + "</font>";
        s += "</td></tr>";
    }
    s += "</table>";
    apxMsg() << s;
}
//=============================================================================
void AppEngine::sleep(quint16 ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec();
}
//=============================================================================
//=============================================================================
QByteArray AppEngine::jsToArray(QJSValue data)
{
    //qDebug()<<portID<<data.toString()<<data.isArray()<<data.toVariant().toByteArray().toHex();
    QByteArray ba;
    if (data.isString() || data.toString().contains(',')) {
        bool ok = false;
        foreach (QString sv,
                 data.toString().trimmed().toLower().split(',', QString::SkipEmptyParts)) {
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
//=============================================================================
QJSValue AppEngine::jsGetProperty(const QString &path)
{
    QStringList list = path.split('.');
    QJSValue v = globalObject();
    for (int i = 0; i < list.size(); ++i) {
        QJSValue vp = v.property(list.at(i));
        if (vp.isUndefined())
            return QJSValue();
        v = vp;
    }
    return v;
}
//=============================================================================
//=============================================================================
QObject *AppEngine::loadQml(const QString &qmlFile, const QVariantMap &opts)
{
    QQmlComponent c(this, qmlFile, QQmlComponent::PreferSynchronous);
    QObject *obj = c.beginCreate(rootContext());
    for (auto key : opts.keys()) {
        QQmlProperty::write(obj, key.toUtf8(), opts.value(key));
    }
    c.completeCreate();
    if (c.isError()) {
        apxMsgW() << c.errorString();
        return nullptr;
    }
    return obj;
}
//=============================================================================
