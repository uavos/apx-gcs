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
#include "ScriptCompiler.h"
#include <App/App.h>
#include <App/AppRoot.h>

ScriptCompiler::ScriptCompiler(QObject *parent)
    : Fact(parent, "script", tr("Script compiler"), "", NoFlags, "cog-sync")
{
    m_version = "11.0";

#if defined(Q_OS_MAC)
    m_platform = "macos";
#elif defined(Q_OS_LINUX)
    m_platform = "linux";
#else
    m_platform = "mingw";
#endif

    m_sdk = QString("wasi-sdk-%1-%2").arg(m_version).arg(m_platform);

    setDescr(m_sdk);

    m_dir = QDir(AppDirs::scripts().absoluteFilePath("compiler"),
                 "wasi-sdk-*",
                 QDir::Name,
                 QDir::Dirs);

    if (!m_dir.exists())
        m_dir.mkpath(".");

    if (!lookup())
        QTimer::singleShot(5000, this, &ScriptCompiler::download);
}

bool ScriptCompiler::lookup()
{
    m_dir.refresh();
    do {
        if (m_dir.entryList().isEmpty())
            break;

        QFileInfo cc(
            QDir(m_dir.entryInfoList().last().absoluteFilePath()).absoluteFilePath("bin/clang"));
        if (!cc.exists())
            break;

        m_cc = cc.absoluteFilePath();
        qDebug() << "found:" << m_cc;

        AppDirs::copyPath(AppDirs::res().absoluteFilePath("scripts/.vscode"),
                          AppDirs::scripts().absoluteFilePath(".vscode"));

        AppDirs::copyPath(AppDirs::res().absoluteFilePath("scripts/sysroot"),
                          AppDirs::scripts().absoluteFilePath("sysroot"));

        AppDirs::copyPath(AppDirs::res().absoluteFilePath("scripts/include"),
                          AppDirs::scripts().absoluteFilePath("include"));

        setEnabled(true);
        return true;
    } while (0);

    qDebug() << "compilers not found";
    setEnabled(false);
    return false;
}

void ScriptCompiler::download()
{
    QString fileName = QString("%1.tar.gz").arg(m_sdk);

    if (QFile::exists(m_dir.absoluteFilePath(fileName))) {
        extract(fileName);
        return;
    }

    apxMsg() << tr("Downloading") << title().append("...");

    QUrl url(QString("https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-%1/%2")
                 .arg(m_version.left(m_version.indexOf('.')))
                 .arg(fileName));

    qDebug() << url;

    net.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkRequest *request = new QNetworkRequest(url);

    QSslConfiguration ssl = request->sslConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    request->setSslConfiguration(ssl);

    request->setRawHeader("Accept", "*/*");
    request->setRawHeader("User-Agent",
                          QString("%1 (v%2)")
                              .arg(QCoreApplication::applicationName())
                              .arg(App::version())
                              .toUtf8());

    setProgress(0);
    QNetworkReply *reply = net.get(*request);
    connect(reply, &QNetworkReply::finished, this, &ScriptCompiler::responseDownload);
    connect(reply, &QNetworkReply::downloadProgress, this, &ScriptCompiler::downloadProgress);
}

void ScriptCompiler::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        setProgress((bytesReceived * 100) / bytesTotal);
    }
}

void ScriptCompiler::responseDownload()
{
    setProgress(-1);

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error()) {
        qWarning() << reply->errorString();
        return;
    }
    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qWarning() << "no data";
        return;
    }

    QString s = reply->header(QNetworkRequest::ContentDispositionHeader).toString();
    s = s.mid(s.lastIndexOf("filename=", Qt::CaseInsensitive));
    s = s.mid(s.indexOf("=") + 1);
    s = s.left(s.indexOf(";"));
    if (!s.startsWith(m_sdk)) {
        apxMsgW() << title().append(':') << tr("Unknown response") << s;
        return;
    }
    apxMsg() << title().append(':') << tr("Received") << s;

    QFile fzip(m_dir.absoluteFilePath(s));
    if (!fzip.open(QFile::WriteOnly)) {
        qWarning() << "can't write" << fzip.fileName();
        return;
    }
    fzip.write(data);
    fzip.close();
    qDebug() << "downloaded" << fzip.fileName();

    // extract
    extract(s);
}

void ScriptCompiler::extract(QString fileName)
{
    qDebug() << "extracting" << fileName;
    setProgress(0);
    QProcess *p = new QProcess();
    connect(p,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &ScriptCompiler::extractFinished);
    p->setWorkingDirectory(m_dir.absolutePath());
    p->start("tar", QStringList() << "xf" << fileName);
}

void ScriptCompiler::extractFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    setProgress(-1);
    QProcess *p = qobject_cast<QProcess *>(sender());
    if (p)
        p->deleteLater();
    if (exitStatus != QProcess::NormalExit || !lookup()) {
        if (p) {
            qWarning() << exitCode << p->errorString();
            qWarning() << p->readAllStandardError();
        }
        apxMsgW() << title() << tr("download error");
        return;
    }
    apxMsg() << title() << tr("available");
}
