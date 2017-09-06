#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QMediaPlayer>
#include "videothread.h"

class FfmpegPlayer : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ videoSurface WRITE setVideoSurface )
    Q_PROPERTY(QString source READ getSource WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int playbackState READ getPlaybackState NOTIFY playbackStateChanged)
    Q_PROPERTY(int error READ getError NOTIFY errorOccured)
    Q_PROPERTY(bool connectingState READ getConnectingState NOTIFY connectingStateChanged)
    Q_PROPERTY(QString errorString READ getErrorString NOTIFY errorStringChanged)
    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();

    static void registerQmlType();

    explicit FfmpegPlayer(QObject *parent = 0);
    ~FfmpegPlayer();


    QAbstractVideoSurface* videoSurface() const;
    void setVideoSurface(QAbstractVideoSurface* s);

    QString getSource() const;
    void setSource(const QString &source);

    int getPlaybackState() const;

    int getError() const;

    QString getErrorString() const;

    bool getConnectingState() const;

private slots:
    void onFrameReceived(QImage frame);
    void onStateChanged(QAbstractSocket::SocketState socketState);
    void onErrorOccurred(QString errorString);
    void setPlaybackState(const QMediaPlayer::State &playbackState);
    void setError(const QMediaPlayer::Error &error);
    void setErrorString(const QString &errorString);
    void setConnectingState(bool connectingState);

private:
    QAbstractVideoSurface* m_surface;
    QVideoSurfaceFormat m_format;
    QString m_source;
    VideoThread m_videoThread;
    int m_playbackState;
    int m_error;
    QString m_errorString;
    bool m_connectingState;
    void presentBlackScreen();

signals:
    void playbackStateChanged();
    void errorOccured();
    void errorStringChanged();
    void sourceChanged();
    void connectingStateChanged();
};

#endif // FFMPEGPLAYER_H
