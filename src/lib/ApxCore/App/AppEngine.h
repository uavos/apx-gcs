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
#ifndef AppEngine_H
#define AppEngine_H
//=============================================================================
#include <Fact/Fact.h>
#include <QJSEngine>
#include <QtCore>
#include <QtQml>
//=============================================================================
class AppEngine : public QQmlApplicationEngine
{
    Q_OBJECT
public:
    explicit AppEngine(QObject *parent = nullptr);
    ~AppEngine();

    QJSValue jsexec(const QString s);
    void jsexec_queued(const QString s);

    void jsSyncObject(QObject *obj);
    void jsSync(Fact *fact);

    Q_INVOKABLE void help();
    Q_INVOKABLE void sleep(quint16 ms);

    Q_INVOKABLE QByteArray jsToArray(QJSValue data);
    QJSValue jsGetProperty(const QString path);
    void jsProtectPropertyWrite(const QString path);
    void jsProtectObjects(const QString path);

    Q_INVOKABLE QObject *loadQml(const QString qmlFile, const QVariantMap &opts);

private:
    QHash<QString, QString> js_descr; //helper commands alias,descr

    QJSValue jsSync(Fact *fact, QJSValue parent); //recursive

    void jsRegister(QString fname, QString description, QString body);

    void jsRegisterFunctions();
    void jsSetProperty(QJSValue parent, QString name, QJSValue value);

    QQueue<QString> _jsQueue;
    QTimer _jsTimer;
    void _queueExec();
    void _protectObjects(const QString path, QJSValue obj);

private slots:
    void warnings(const QList<QQmlError> &warnings);
};
//=============================================================================
#endif
