#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDateTime>
#include <QGeoCoordinate>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QThread>
#include <QVector>

class OverlayTileWorker : public QObject
{
    Q_OBJECT

public:
    explicit OverlayTileWorker(QObject *parent = nullptr);

public slots:
    void resetSession(
        const QString &sessionDir,
        int nativeZoom,
        int minZoom,
        int tileSize
    );

    void drawSample(
        double lat,
        double lon,
        double altitude,
        const QString &source
    );

    void drawSegment(
        double lat1,
        double lon1,
        double lat2,
        double lon2,
        double altitude,
        const QString &source
    );

    void flush();

signals:
    void tileUpdated(
        int z,
        int x,
        int y,
        const QString &source,
        const QString &color,
        const QString &path
    );

private:
    struct DirtyTile
    {
        int z = 0;
        int x = 0;
        int y = 0;
        QString source;
        QString color;
        QString path;
    };

    double footprintMeters(double altitude) const;

    void drawFootprint(
        double lat,
        double lon,
        double altitude,
        const QString &source
    );

    void drawFootprintIntoTile(
        int z,
        int x,
        int y,
        const QRectF &globalRect,
        const QColor &color,
        const QString &source
    );

    void maybeFlush();

    void buildPyramidFromChild(
        int childZ,
        int childX,
        int childY,
        QSet<QString> *rebuilt
    );

    void buildParentTile(
        int z,
        int x,
        int y,
        QSet<QString> *rebuilt
    );

    static QString tileKey(int z, int x, int y);

    static double lonToGlobalPixelX(double lon, int z);
    static double latToGlobalPixelY(double lat, int z);
    static double metersPerPixel(double lat, int z);

    QString colorForSource(const QString &source) const;

    QString tilePath(
        int z,
        int x,
        int y,
        bool createDir = true
    ) const;

    void trimCache();

    QString _sessionDir;

    int _nativeZoom = 16;
    int _minZoom = 12;
    int _tileSize = 256;

    QHash<QString, QImage> _tileCache;
    QHash<QString, DirtyTile> _dirtyTiles;

    qint64 _lastFlushMs = 0;

    int _flushIntervalMs = 300;
    int _maxDirtyBeforeFlush = 32;
    int _maxCachedTiles = 256;
};

class OverlayTileModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int displayZoom READ displayZoom NOTIFY displayZoomChanged)
    Q_PROPERTY(int minZoom READ minZoom CONSTANT)
    Q_PROPERTY(int maxZoom READ maxZoom CONSTANT)

public:
    struct OverlayTile
    {
        int z = 16;
        int x = 0;
        int y = 0;

        double north = 0.0;
        double south = 0.0;
        double west = 0.0;
        double east = 0.0;

        QString source;
        QString color;
        QString time;
        QString tileUrl;

        int revision = 0;
    };

    enum Roles {
        ZRole = Qt::UserRole + 1,
        XRole,
        YRole,
        NorthRole,
        SouthRole,
        WestRole,
        EastRole,
        SourceRole,
        ColorRole,
        TimeRole,
        TileUrlRole,
        RevisionRole
    };

    explicit OverlayTileModel(QObject *parent = nullptr);
    ~OverlayTileModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(
        const QModelIndex &index,
        int role = Qt::DisplayRole
    ) const override;

    QHash<int, QByteArray> roleNames() const override;

    int displayZoom() const { return _displayZoom; }
    int minZoom() const { return _minZoom; }
    int maxZoom() const { return _nativeZoom; }

    Q_INVOKABLE void setDisplayZoom(int zoom);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void startNewSession();
    Q_INVOKABLE void flush();

    Q_INVOKABLE QString sessionId() const { return _sessionId; }

    Q_INVOKABLE QString sessionDirPath() const
    {
        return sessionDir();
    }

    Q_INVOKABLE int nativeZoomLevel() const
    {
        return _nativeZoom;
    }

    Q_INVOKABLE int minZoomLevel() const
    {
        return _minZoom;
    }

    void addSample(
        double lat,
        double lon,
        double altitude,
        const QString &source
    );

    void addSegment(
        double lat1,
        double lon1,
        double lat2,
        double lon2,
        double altitude,
        const QString &source
    );

signals:
    void displayZoomChanged();

    void workerResetSession(
        const QString &sessionDir,
        int nativeZoom,
        int minZoom,
        int tileSize
    );

    void workerDrawSample(
        double lat,
        double lon,
        double altitude,
        const QString &source
    );

    void workerDrawSegment(
        double lat1,
        double lon1,
        double lat2,
        double lon2,
        double altitude,
        const QString &source
    );

private slots:
    void onWorkerTileUpdated(
        int z,
        int x,
        int y,
        const QString &source,
        const QString &color,
        const QString &path
    );

private:
    void rebuildVisible();

    static QString tileKey(int z, int x, int y);

    static double tileXToLon(int x, int z);
    static double tileYToLat(int y, int z);

    QString storageRoot() const;
    QString sessionDir() const;

    void writeManifest() const;

    QHash<QString, OverlayTile> _tiles;
    QVector<QString> _visibleKeys;
    QHash<QString, int> _visibleRowByKey;

    QString _sessionId;

    int _nativeZoom = 16;
    int _minZoom = 12;
    int _displayZoom = 16;
    int _tileSize = 256;

    QThread _workerThread;
    OverlayTileWorker *_worker = nullptr;
};