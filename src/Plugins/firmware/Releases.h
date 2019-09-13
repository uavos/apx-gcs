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
#ifndef Releases_H
#define Releases_H
//=============================================================================
#include "Firmware.h"
#include <Fact/Fact.h>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
//=============================================================================
class Releases : public Fact
{
    Q_OBJECT

public:
    explicit Releases(Fact *parent);

    Fact *f_sync;
    Fact *f_current;
    Fact *f_dev;

    bool loadFirmware(QString nodeName,
                      QString hw,
                      Firmware::UpgradeType type,
                      QByteArray *data,
                      quint32 *startAddr);

    QString releaseVersion() const;

private:
    QNetworkAccessManager net;

    QString m_releaseFile;

    QString m_packagePrefix;

    QNetworkReply *reply;
    QNetworkReply *request(const QString &r);
    QNetworkReply *request(const QUrl &url);

    QDir releaseDir() const;
    QDir devDir() const;

    bool extractRelease(const QString &fname);
    void makeReleaseFact(const QDir &dir);
    void makeReleaseFactDo(Fact *fact, const QDir &dir);
    void clean();

    QNetworkReply *checkReply(QObject *sender);
    bool isHttpRedirect(QNetworkReply *reply);

    bool isFirmwarePackageFile(const QString &s, const QString &ver = QString());

    QString getApxfwFileName(QString nodeName, QString hw);
    bool loadHexFile(QString fileName, QByteArray *data, quint32 *startAddr);
    bool loadApfwFile(QString fileName, QString section, QByteArray *data, quint32 *startAddr);
    bool loadFileMHX(QString ver, QByteArray *data);

private slots:
    void abort();
    void sync();

    void requestRelease(QString req);
    void responseRelease();

    void requestDownload(QUrl url);
    void responseDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};
//=============================================================================
#endif
