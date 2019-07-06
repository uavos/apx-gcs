#ifndef GSTPLAYER_H
#define GSTPLAYER_H

#include <QtCore>
#include <QAbstractVideoSurface>
#include <Fact/Fact.h>
#include "videothread.h"
#include "overlay.h"

class GstPlayer : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface *videoSurface MEMBER m_videoSurface)
    Q_PROPERTY(ConnectionState connectionState READ getConnectionState NOTIFY connectionStateChanged)

public:
    static const int THREAD_STOP_TIMEOUT = 500;
    static const int RECONNECT_TIMEOUT = 5000;
    enum ConnectionState {
        STATE_UNCONNECTED,
        STATE_CONNECTING,
        STATE_CONNECTED
    };
    enum MediaType {
        mtImage,
        mtVideo
    };

    Q_ENUM(ConnectionState)
    explicit GstPlayer(Fact *parent = nullptr);
    ~GstPlayer() override;

    Fact *f_visible;
    Fact *f_tune;
    Fact *f_active;
    Fact *f_record;
    Fact *f_reencoding;
    Fact *f_uri;
    Overlay *f_overlay;

    ConnectionState getConnectionState() const;

    Q_INVOKABLE void snapshot() const;

    static QString getMediaFileName(MediaType type);

private:
    QAbstractVideoSurface *m_videoSurface = nullptr;
    VideoThread m_videoThread;
    QImage m_lastFrame;
    ConnectionState m_connectionState = STATE_UNCONNECTED;
    QTimer m_reconnectTimer;

    void setConnectionState(ConnectionState cs);

    void play();
    void stop();

private slots:
    void onFrameReceived(const QImage &image);
    void onActiveValueChanged();
    void onRecordValueChanged();
    void onReencodingValueChanged();
    void onErrorOccured(const QString &error) const;
    void onReconnectTimerTimeout();

signals:
    void connectionStateChanged();
};

#endif //GSTPLAYER_H
