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
#include "AppLog.h"
#include "AppDirs.h"
//=============================================================================
APX_LOGGING_CATEGORY(ApplicationLog, "app")
APX_LOGGING_CATEGORY(ConsoleLog, "console")
//=============================================================================
static QtMessageHandler messageHandlerChain = nullptr;
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
AppLog *AppLog::_instance = nullptr;
QMutex AppLog::_mutex;
//=============================================================================
AppLog::AppLog(QObject *parent)
    : QObject(parent)
    , appLogStream(nullptr)
{
    _instance = this;

    if (!AppDirs::logs().exists())
        AppDirs::logs().mkpath(".");
    QFile *f = new QFile(AppDirs::logs().absoluteFilePath("APX.txt"), this);
    if (!f->open(QFile::WriteOnly | QFile::Truncate)) {
        apxMsgW() << "Cant write log file" << f->fileName();
        f->deleteLater();
    } else {
        appLogStream = new QTextStream(f);
    }

#if (defined QT_DEBUG) || 1
    //qSetMessagePattern("[%{time yyyyMMdd h:mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{if-category}%{category}%{endif}%{if-debug} - %{function}%{endif}%{if-warning} - %{function}%{endif} - %{message}");
    qSetMessagePattern(
        "[%{time yyyyMMdd h:mm:ss.zzz} "
        "%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-"
        "fatal}F%{endif}] %{if-category}%{category}%{endif} - %{function} - %{message}");
#else
    qSetMessagePattern("[%{time yyyyMMdd h:mm:ss.zzz} "
                       "%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}"
                       "C%{endif}%{if-fatal}F%{endif}] %{category} - %{message}");
#endif

    messageHandlerChain = qInstallMessageHandler(messageHandler);
}
//============================================================================
AppLog::~AppLog()
{
    qInstallMessageHandler(messageHandlerChain);

    if (appLogStream) {
        appLogStream->flush();
        appLogStream->device()->close();
        delete appLogStream;
        appLogStream = nullptr;
    }

    foreach (QTextStream *stream, streams.values()) {
        stream->flush();
        stream->device()->close();
        delete stream;
    }
}
//=============================================================================
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    AppLog::instance()->message(type, context, message);
}
//============================================================================
//============================================================================
void AppLog::add(const QString &categoryName, const QString &fileName, bool silent)
{
    QDir dir(QFileInfo(AppDirs::logs().absoluteFilePath(fileName)).absoluteDir());
    if (!dir.exists())
        dir.mkpath(".");

    QFile *f = new QFile(AppDirs::logs().absoluteFilePath(fileName), AppLog::_instance);
    if (!f->open(QFile::WriteOnly | QFile::Truncate)) {
        apxMsgW() << "Cant write log file" << fileName;
        f->deleteLater();
        return;
    }
    QTextStream *stream = new QTextStream(f);
    QMutexLocker lock(&AppLog::_mutex);
    AppLog::_instance->streams.insert(categoryName, stream);
    if (silent)
        AppLog::_instance->silentCategories.append(categoryName);
}
//=============================================================================
void AppLog::message(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (!silentCategories.contains(context.category))
        messageHandlerChain(type, context, message);

    QMutexLocker lock(&_mutex);

    QString msg = qFormatLogMessage(type, context, message);
    if (!msg.endsWith('\n'))
        msg.append('\n');

    QTextStream *stream = streams.value(context.category);
    if (stream) {
        *stream << qPrintable(msg);
        stream->flush();
    } else if (appLogStream) {
        *appLogStream << qPrintable(msg);
        appLogStream->flush();
    }
    if (silentCategories.contains(context.category))
        return;

    emit consoleMessage(message);

    if (display(context)) {
        switch (type) {
        default:
            emit infoMessage(message);
            break;
        case QtInfoMsg:
            emit infoMessage(message);
            break;
        case QtWarningMsg:
            emit warningMessage(message);
            break;
        }
    }
}
//=============================================================================
bool AppLog::display(const QMessageLogContext &context)
{
    QString cat(context.category);
    if (cat == ApplicationLog().categoryName())
        return true;
    if (cat == "qml")
        return true; //console from qml and jsengine
    //if(cat==ConsoleLog().categoryName())return true;
    return false;
}
//=============================================================================
//=============================================================================
