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

#include <QtCore>
#include <QtNetwork>

#include <App/AppDirs.h>
#include <Fact/Fact.h>

class TileLoader : public Fact
{
    Q_OBJECT
    Q_ENUMS(MapID)

    Q_PROPERTY(int requestCount READ requestCount NOTIFY requestCountChanged)

public:
    TileLoader(Fact *parent = nullptr);
    ~TileLoader();

    static TileLoader *instance() { return _instance; }

    Fact *f_offline;

    // download visible area to cache
    Fact *f_area; // group, value is the estimated tiles count
    Fact *f_zoom;
    Fact *f_cache;
    Fact *f_download;
    Fact *f_stop;

    // visible map area, reported by the map view
    // map type is matched by description ("hybrid map", "satellite map")
    Q_INVOKABLE void setVisibleArea(
        double latN, double lonW, double latS, double lonE, int zoom, QString mapType);

    enum MapID {
        GoogleHybrid,
        GoogleSatellite,
    };
    Q_ENUM(MapID)

    static inline quint64 uid(quint8 type, quint8 level, quint32 x, quint32 y)
    {
        return ((quint64) type << 56) | ((quint64) level << 48) | ((quint64) x << 24) | (quint64) y;
    }
    static inline quint8 type(quint64 uid) { return (uid >> 56) & (((quint64) 1 << 8) - 1); }
    static inline quint8 level(quint64 uid) { return (uid >> 48) & (((quint64) 1 << 8) - 1); }
    static inline quint32 x(quint64 uid) { return (uid >> 24) & (((quint64) 1 << 24) - 1); }
    static inline quint32 y(quint64 uid) { return uid & (((quint64) 1 << 24) - 1); }
    static inline quint64 dbHash(quint64 uid) { return uid & (((quint64) 1 << 56) - 1); }

    static auto basename(quint64 uid)
    {
        return QString("%1%2")
            .arg(x(uid), 6, 16, QChar('0'))
            .arg(y(uid), 6, 16, QChar('0'))
            .toUpper();
    }
    static auto cachePath() { return AppDirs::storage().absoluteFilePath("maps"); }
    static auto basePath(quint64 uid)
    {
        return QStringList({cachePath(),
                            "google",
                            QString(QMetaEnum::fromType<MapID>().valueToKey(type(uid)))})
            .join('/');
    }
    static int tileX(double lon, int zoom);
    static int tileY(double lat, int zoom);

protected:
    void updateStatus();

private:
    static TileLoader *_instance;

    //downloader
    QNetworkAccessManager *net;
    QByteArray userAgent;
    QString language;
    QString versionGoogleMaps;
    QNetworkReply *downloadRequest(QNetworkRequest *request);
    QHash<QNetworkReply *, quint64> reqMap;

    QList<quint64> pendingDownloads;
    QList<quint64> downloads;

    int getServerNum(int x, int y, int max);
    void getSecGoogleWords(int x, int y, QString &sec1, QString &sec2);
    bool checkGoogleVersion(QNetworkRequest *request);

    bool checkImage(const QByteArray &data);

    int requestCount() { return reqMap.size(); }

    QString getFileName(quint64 uid);

    // read tile from the cache on disk
    bool readTile(quint64 uid, QByteArray *data);

    // visible area
    double _visLatN{}, _visLonW{}, _visLatS{}, _visLonE{};
    int _visZoom{};
    QString _visMapType;
    int areaTilesCount(int zoomFrom, int zoomTo) const;
    int areaMissingCount(int zoomFrom, int zoomTo, MapID type) const;
    void updateAreaInfo();

    // area download queue
    QList<quint64> _areaQueue;
    QSet<quint64> _areaActive;
    QSet<quint64> _areaRetried;
    int _areaTotal{};
    int _areaDone{};
    void areaNext();
    void areaFinished(quint64 uid);
    void areaError(quint64 uid);
    void updateAreaStatus();

private slots:
    void download(quint64 uid);

    void networkReplyError(QNetworkReply::NetworkError error);
    void networkReplyFinished();
    void versionReplyFinished();

    void downloadArea();
    void areaStop();
    void updateCacheInfo();

public slots:
    void loadTile(quint64 uid);
    void saveTile(quint64 uid, uint version, const QByteArray &data);
    void loadCancel(quint64 uid);
    void abort();

signals:
    void tileLoaded(quint64 uid, QByteArray data);
    void tileError(quint64 uid, QString errorString);

    void requestCountChanged();
};
