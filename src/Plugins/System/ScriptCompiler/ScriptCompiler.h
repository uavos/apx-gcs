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

#include <App/AppNotify.h>
#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
#include <QtCore>

#include <GithubReleases.h>

class ScriptCompiler : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QString cc MEMBER m_cc);
    Q_PROPERTY(Fact *use_vscode MEMBER f_vscode);

public:
    explicit ScriptCompiler(QObject *parent = nullptr);

    Fact *f_vscode;
    Fact *f_cc;
    Fact *f_llvm_path;

private:
    GithubReleases _gh{"WebAssembly/wasi-sdk", this};

    bool lookup();
    bool lookup_llvm();
    bool lookup_wasi();

    QString m_cc;

    QString m_tag;
    QString m_sdk;

    QDir m_dir;

    bool _justExtracted{};

    void extract(QString fileName);

    void setCompiler(QString cc);

private slots:
    void lookup_init();

    void download();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(QString assetName, QFile *data);

    void extractFinished(int exitCode, QProcess::ExitStatus exitStatus);

signals:
    void available();
};
