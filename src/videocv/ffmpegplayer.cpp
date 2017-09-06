#include "ffmpegplayer.h"

#include <QtQml/qqml.h>

#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>

void FfmpegPlayer::registerQmlType()
{
    qmlRegisterType<FfmpegPlayer>("FfmpegPlayer", 0, 1, "FfmpegPlayer");
}

FfmpegPlayer::FfmpegPlayer(QObject *parent):
    QObject(parent),
    m_surface(0),
    m_playbackState(QMediaPlayer::StoppedState),
    m_error(QMediaPlayer::NoError),
    m_errorString(""),
    m_connectingState(false)
{
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    connect(&m_videoThread, SIGNAL(errorOccured(QString)), this, SLOT(onErrorOccurred(QString)));
    connect(&m_videoThread, SIGNAL(frameReceived(QImage)), this, SLOT(onFrameReceived(QImage)));
    connect(&m_videoThread, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(onStateChanged(QAbstractSocket::SocketState)));
}

void FfmpegPlayer::play()
{
    setError(QMediaPlayer::NoError);
    setErrorString("");
    m_videoThread.setInputUrl(m_source);
    m_videoThread.start();
}

void FfmpegPlayer::stop()
{
    setPlaybackState(QMediaPlayer::StoppedState);
    m_videoThread.stop();
    if(m_videoThread.wait(300))
        m_videoThread.terminate();
    if(m_surface)
        m_surface->stop();
}

FfmpegPlayer::~FfmpegPlayer()
{
    stop();
}

QAbstractVideoSurface* FfmpegPlayer::videoSurface() const
{
    return m_surface;
}

void FfmpegPlayer::setVideoSurface(QAbstractVideoSurface* s)
{
    m_surface = s;
}

void FfmpegPlayer::onFrameReceived(QImage frame)
{
    if(!m_surface)
        return;
    if(!m_surface->isActive())
    {
        m_surface->start(QVideoSurfaceFormat(frame.size(), QVideoFrame::pixelFormatFromImageFormat(frame.format())));
    }
    m_surface->present(QVideoFrame(frame));
}

void FfmpegPlayer::onStateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState == QAbstractSocket::ConnectedState)
    {
        setConnectingState(false);
        setPlaybackState(QMediaPlayer::PlayingState);
    }
    else if(socketState == QAbstractSocket::UnconnectedState)
    {
        setConnectingState(false);
        setPlaybackState(QMediaPlayer::StoppedState);
        presentBlackScreen();
    }
    else if(socketState == QAbstractSocket::ConnectingState)
    {
        setConnectingState(true);
    }
}

void FfmpegPlayer::onErrorOccurred(QString errorString)
{
    setErrorString(errorString);
    setError(QMediaPlayer::ServiceMissingError);
    stop();
}

QString FfmpegPlayer::getErrorString() const
{
    return m_errorString;
}

void FfmpegPlayer::setErrorString(const QString &errorString)
{
    bool needSignal = (m_errorString != errorString);
    m_errorString = errorString;
    if(needSignal)
        emit errorStringChanged();
}

bool FfmpegPlayer::getConnectingState() const
{
    return m_connectingState;
}

void FfmpegPlayer::setConnectingState(bool connectingState)
{
    bool needSignal = (m_connectingState != connectingState);
    m_connectingState = connectingState;
    if(needSignal)
        emit connectingStateChanged();
}

void FfmpegPlayer::presentBlackScreen()
{
    if(m_surface && !m_surface->surfaceFormat().frameSize().isEmpty())
    {
        QImage blackImage(m_surface->surfaceFormat().frameSize(), QImage::Format_RGB32);
        blackImage.fill(Qt::black);
        onFrameReceived(blackImage);
    }
}

int FfmpegPlayer::getError() const
{
    return m_error;
}

void FfmpegPlayer::setError(const QMediaPlayer::Error &error)
{
    bool needSignal = (m_error != error);
    m_error = error;
    if(needSignal)
        emit errorOccured();
}

int FfmpegPlayer::getPlaybackState() const
{
    return m_playbackState;
}

void FfmpegPlayer::setPlaybackState(const QMediaPlayer::State &playbackState)
{
    bool needSignal = (m_playbackState != playbackState);
    m_playbackState = playbackState;
    if(needSignal)
        emit playbackStateChanged();
}

QString FfmpegPlayer::getSource() const
{
    return m_source;
}

void FfmpegPlayer::setSource(const QString &source)
{
    bool needSignal = (m_source != source);
    m_source = source;
    if(needSignal)
        emit sourceChanged();
}
