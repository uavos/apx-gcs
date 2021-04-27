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

#include <Fact/Fact.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class ApxFw : public Fact
{
    Q_OBJECT

public:
    explicit ApxFw(Fact *parent);

    Fact *f_sync;
    Fact *f_current{nullptr};
    Fact *f_dev{nullptr};

    bool loadFirmware(
        QString nodeName, QString hw, QString type, QByteArray *data, quint32 *startAddr);

    QJsonArray loadParameters(QString nodeName, QString hw);

private:
    QNetworkAccessManager net;

    QVersionNumber _versionPrefix;

    QString m_packagePrefix;

    QNetworkReply *reply{nullptr};
    QNetworkReply *request(const QString &r);
    QNetworkReply *request(const QUrl &url);

    QDir releaseDir() const;
    QDir devDir() const;

    bool extractRelease(const QString &filePath);
    void makeFacts(Fact *fact, QDir dir);
    void clean();

    QNetworkReply *checkReply(QObject *sender);
    bool isHttpRedirect(QNetworkReply *reply);

    bool isFirmwarePackageFile(const QString &s);

    QString getApxfwFileName(QString nodeName, QString hw);
    bool loadApfwFile(QString fileName, QString section, QByteArray *data, quint32 *startAddr);

    bool loadHexFile(QString fileName, QByteArray *data, quint32 *startAddr);
    bool loadFileMHX(QString ver, QByteArray *data);

    void updateNodesMeta(QDir dir);
    void updateNodesMeta(QVariantMap &meta, QString version, QJsonValue json, QStringList path);

private slots:
    void abort();
    void sync();
    void syncFacts();

    void updateCurrent();

    // internet
    void requestLatestTag();
    void responseLatestTag();

    void requestRelease(QString req);
    void responseRelease();

    void requestDownload(QUrl url);
    void responseDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};
