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

#include <QLoggingCategory>
#include <QMessageLogContext>
#include <QMessageLogger>
#include <QtCore>

Q_DECLARE_LOGGING_CATEGORY(ApplicationLog)
Q_DECLARE_LOGGING_CATEGORY(ConsoleLog)

#define APX_LOGGING_CATEGORY(name, ...) Q_LOGGING_CATEGORY(name, __VA_ARGS__)

#ifndef APX_LOGGING_STREAM_MOD
#define APX_LOGGING_STREAM_MOD .noquote()
#endif

#define apxDebug() qDebug() APX_LOGGING_STREAM_MOD

#define apxMsg() \
    QMessageLogger(QT_MESSAGELOG_FILE, \
                   QT_MESSAGELOG_LINE, \
                   QT_MESSAGELOG_FUNC, \
                   ApplicationLog().categoryName()) \
        .info() APX_LOGGING_STREAM_MOD

#define apxMsgW() \
    QMessageLogger(QT_MESSAGELOG_FILE, \
                   QT_MESSAGELOG_LINE, \
                   QT_MESSAGELOG_FUNC, \
                   ApplicationLog().categoryName()) \
        .warning() APX_LOGGING_STREAM_MOD

#define apxConsole() \
    QMessageLogger(QT_MESSAGELOG_FILE, \
                   QT_MESSAGELOG_LINE, \
                   QT_MESSAGELOG_FUNC, \
                   ConsoleLog().categoryName()) \
        .info() APX_LOGGING_STREAM_MOD

#define apxConsoleW() \
    QMessageLogger(QT_MESSAGELOG_FILE, \
                   QT_MESSAGELOG_LINE, \
                   QT_MESSAGELOG_FUNC, \
                   ConsoleLog().categoryName()) \
        .warning() APX_LOGGING_STREAM_MOD

class AppLog : public QObject
{
    Q_OBJECT
public:
    explicit AppLog(QObject *parent = nullptr);
    ~AppLog();
    static AppLog *instance() { return _instance; }

    void message(QtMsgType type, const QMessageLogContext &context, const QString &message);

    enum Options {
        Display,
    };

    static void add(const QString &categoryName, const QString &fileName, bool silent);
    static bool display(const QMessageLogContext &context);

private:
    static AppLog *_instance;
    static QMutex _mutex;
    QMap<QString, QTextStream *> streams; //category -> file

    QList<QString> silentCategories; //no print to stdout

    QTextStream *appLogStream;

    //notifications support
signals:
    void infoMessage(QString msg);
    void warningMessage(QString msg);
    void consoleMessage(QString msg);
};
