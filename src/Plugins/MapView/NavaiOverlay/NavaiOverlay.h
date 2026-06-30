#pragma once

#include <Fact/Fact.h>

#include <QAbstractListModel>
#include <QHash>
#include <QHostAddress>
#include <QJsonObject>
#include <QTimer>
#include <QUdpSocket>
#include <QtCore>

class Unit;

class NavaiResultModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        LatitudeRole = Qt::UserRole + 1,
        LongitudeRole,
        RadiusMetersRole,
        PercentRole,
        LabelRole,
        ItemOpacityRole
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
        double radiusMeters,
        double percent,
        const QString &label
    );

    Q_INVOKABLE void clear();

private slots:
    void updateFade();

private:
    struct Result
    {
        double lat = 0.0;
        double lon = 0.0;
        double radiusMeters = 0.0;
        double percent = 0.0;
        double opacity = 0.0;
        double targetOpacity = 1.0;
        QString label;
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
    Q_PROPERTY(Fact *enableFact READ enableFact CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool udpReady READ udpReady NOTIFY udpReadyChanged)
    Q_PROPERTY(quint16 udpPort READ udpPort CONSTANT)

public:
    explicit NavaiOverlay(Fact *parent = nullptr);
    ~NavaiOverlay() override = default;

    QAbstractListModel *resultsModel()
    {
        return &_resultsModel;
    }

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

signals:
    void activeChanged();
    void udpReadyChanged();

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

    Fact *f_enabled = nullptr;

    Fact *f_camLat = nullptr;
    Fact *f_camLon = nullptr;

    QHash<QString, Fact *> _mandalaFacts;

    NavaiResultModel _resultsModel;

    QUdpSocket *_udpSocket = nullptr;

    bool _active = false;
    bool _udpReady = false;
    quint16 _udpPort = 9300;
};
