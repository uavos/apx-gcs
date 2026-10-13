#pragma once

#include <Fact/Fact.h>

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QUdpSocket>
#include <QtCore>

#include "NavaiTileServer.h"

class Unit;
class NavaiTileModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { IdRole = Qt::UserRole + 1, PolygonRole, ColorRole };

    explicit NavaiTileModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : _items.size();
    }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // The whole grid from NAVAI: replace it or merge new tiles into it.
    void replace(const QVector<QVariantMap> &tiles);
    void merge(const QVector<QVariantMap> &tiles);
    // Scores for some of the tiles; tiles not named keep their current score.
    void score(const QHash<QString, QVariantMap> &scores);

    QVariantList heatmapTiles() const;
    const QVector<QVariantMap> &tiles() const { return _items; }

private:
    QVector<QVariantMap> _items;
    QHash<QString, int> _rows; // tile id -> row in _items
};

class NavaiResultModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        LatitudeRole = Qt::UserRole + 1,
        LongitudeRole,
        PercentRole,
        LabelRole,
        ItemOpacityRole,
        TrajectoryCoordinatesRole,
        LatestRole
    };

    explicit NavaiResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(
        const QModelIndex &index,
        int role = Qt::DisplayRole
    ) const override;

    QHash<int, QByteArray> roleNames() const override;

    void addResult(
        double lat,
        double lon,
        double percent,
        const QString &label,
        const QVariantList &trajectoryCoordinates
    );

    Q_INVOKABLE void clear();

private slots:
    void updateFade();

private:
    struct Result
    {
        double lat = 0.0;
        double lon = 0.0;
        double percent = 0.0;
        double opacity = 0.0;
        double targetOpacity = 1.0;
        QString label;
        QVariantList trajectoryCoordinates;
    };

    void startFadeTimer();

    QVector<Result> _items;

    QTimer _fadeTimer;
    qint64 _lastFadeMs = 0;

    int _fadeDurationMs = 1800;
};

class NavaiOverlay : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel *resultsModel READ resultsModel CONSTANT)
    Q_PROPERTY(QAbstractListModel *tileGridModel READ tileGridModel CONSTANT)
    Q_PROPERTY(QVariantList heatmapTiles READ heatmapTiles NOTIFY heatmapChanged)
    Q_PROPERTY(Fact *enableFact READ enableFact CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool udpReady READ udpReady NOTIFY udpReadyChanged)
    Q_PROPERTY(quint16 udpPort READ udpPort CONSTANT)
    Q_PROPERTY(QString overlayTileUrlTemplate READ overlayTileUrlTemplate NOTIFY overlayTileServerChanged)
    Q_PROPERTY(bool overlayTileServerReady READ overlayTileServerReady NOTIFY overlayTileServerChanged)
    Q_PROPERTY(QVariantList opticalTrack READ opticalTrack NOTIFY opticalTrackChanged)

public:
    explicit NavaiOverlay(Fact *parent = nullptr);
    ~NavaiOverlay() override = default;

    QAbstractListModel *resultsModel()
    {
        return &_resultsModel;
    }
    QAbstractListModel *tileGridModel() { return &_tileGridModel; }
    QVariantList heatmapTiles() const { return _tileGridModel.heatmapTiles(); }

    Fact *enableFact() const
    {
        return f_enabled;
    }

    bool active() const
    {
        return _active;
    }

    bool udpReady() const
    {
        return _udpReady;
    }

    quint16 udpPort() const
    {
        return _udpPort;
    }
    QString overlayTileUrlTemplate() const { return _tileServer.urlTemplate(); }
    bool overlayTileServerReady() const { return _tileServer.isListening(); }

    // The optical flow track: one list of coordinates per flow chain.
    QVariantList opticalTrack() const;

signals:
    void activeChanged();
    void udpReadyChanged();
    void heatmapChanged();
    void overlayTileServerChanged();
    void opticalTrackChanged();

private slots:
    void readUdpDatagrams();
    void updateEnabled();

    void bindUnit(Unit *unit);

private:
    QString uiDir() const;

    void setupUdp();
    void stopUdp();

    bool isOverlayEnabled() const;

    void handleDatagram(
        const QByteArray &data,
        const QHostAddress &sender,
        quint16 senderPort
    );
    void handleTileGrid(const QJsonObject &);
    void handleDinoRanking(const QJsonObject &);
    void handleOpticalFlow(const QJsonObject &);

    double jsonNumber(
        const QJsonObject &obj,
        const QStringList &keys,
        bool *ok
    ) const;

    void writeCameraFacts(
        double lat,
        double lon
    );

    void postToGcsConsole(const QString &text);

    QVariantList parseTrajectory(const QJsonArray &trajectory) const;

    Fact *f_enabled = nullptr;

    Fact *f_camLat = nullptr;
    Fact *f_camLon = nullptr;

    QHash<QString, Fact *> _mandalaFacts;

    Unit *_unit = nullptr;

    NavaiResultModel _resultsModel;
    NavaiTileModel _tileGridModel;
    NavaiTileServer _tileServer;
    QHash<int,QJsonObject> _gridChunks;

    QVector<QList<QGeoCoordinate>> _opticalSegments;
    int _opticalPoints = 0;
    static constexpr int kMaxOpticalPoints = 3000;
    int _gridChunkCount=0;
    QString _gridRevision;
    QString _gridMode;

    QUdpSocket *_udpSocket = nullptr;

    bool _active = false;
    bool _udpReady = false;

    quint16 _udpPort = 5005;
};
