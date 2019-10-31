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
    , logStream(nullptr)
{
    _instance = this;

    if (!AppDirs::logs().exists())
        AppDirs::logs().mkpath(".");
    QFile *f = new QFile(AppDirs::logs().absoluteFilePath("APX.txt"), this);
    if (!f->open(QFile::WriteOnly | QFile::Truncate)) {
        apxMsgW() << "Cant write log file" << f->fileName();
        f->deleteLater();
    } else {
        logStream = new QTextStream(f);
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

    if (logStream) {
        logStream->flush();
        logStream->device()->close();
        delete logStream;
        logStream = nullptr;
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
    messageHandlerChain(type, context, message);
    AppLog::instance()->message(type, context, message);
}
//============================================================================
//============================================================================
void AppLog::add(const QString &categoryName, const QString &fileName, bool debug)
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
    if (debug)
        AppLog::_instance->debugStreams.append(stream);
}
//=============================================================================
void AppLog::message(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QMutexLocker lock(&_mutex);

    QString msg = qFormatLogMessage(type, context, message);
    if (!msg.endsWith('\n'))
        msg.append('\n');

    QTextStream *stream = streams.value(context.category);
    if (stream) {
        *stream << qPrintable(msg);
        stream->flush();
    }

    emit consoleMessage(message);

    if (display(context)) {
        switch (type) {
        default:
            break;
        case QtInfoMsg:
            emit infoMessage(message);
            break;
        case QtWarningMsg:
            emit warningMessage(message);
            break;
        }
    }

    if (debugStreams.contains(stream))
        return;
    if (logStream) {
        *logStream << qPrintable(msg);
        logStream->flush();
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
