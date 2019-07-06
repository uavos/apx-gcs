#include "gstplayer.h"

#include <QVideoSurfaceFormat>
#include <QtQml>
#include "App/AppSettings.h"
#include "Vehicles/Vehicle.h"
#include "ApxLog.h"
#include "ApxApp.h"
#include "ApxDirs.h"

using namespace std::placeholders;

GstPlayer::GstPlayer(Fact *parent)
    : Fact(parent, "video", tr("Video"), tr("Camera link and streaming"), Group)
{
    if(!ApxDirs::images().exists())
        ApxDirs::images().mkpath(ApxDirs::images().absolutePath());
    if(!ApxDirs::video().exists())
        ApxDirs::video().mkpath(ApxDirs::video().absolutePath());

    qmlRegisterType<GstPlayer>("GstPlayer", 1, 0, "GstPlayer");

    setIcon("video");

    QSettings *settings = AppSettings::settings();

    f_visible = new AppSettingFact(settings, this, "show_window", tr("Visible"), tr("Show video window"), Bool, false);
    f_visible->setIcon("check");

    f_tune = new Fact(this, "tune", tr("Tune"), tr("Video stream settings"), Group);
    f_tune->setIcon("tune");

    f_active = new Fact(f_tune, "running", tr("Active"), tr("Receive video stream"), Bool);
    f_active->setIcon("video-input-antenna");

    f_record = new Fact(f_tune, "record", tr("Record"), tr("Save stream to file"), Bool);
    f_record->setIcon("record-rec");

    f_reencoding = new AppSettingFact(settings, f_tune, "reencoding", tr("Reencoding"), tr("Video reencoding"),
                                      Bool, false);
    f_reencoding->setIcon("film");

    f_uri = new AppSettingFact(
                settings,
                f_tune,
                "uri",
                tr("URI"),
                tr("file://<...>, rtsp://<...> or other"),
                Text,
                QString("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov"));
    f_uri->setIcon("link");

    f_overlay = new Overlay(f_tune);
    f_overlay->setIcon("image-plus");

    connect(&m_videoThread, &VideoThread::frameReceived, this, &GstPlayer::onFrameReceived);
    connect(&m_videoThread, &VideoThread::errorOccured, this, &GstPlayer::onErrorOccured);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &GstPlayer::onReconnectTimerTimeout);
    connect(f_active, &Fact::valueChanged, this, &GstPlayer::onActiveValueChanged);
    connect(f_record, &Fact::valueChanged, this, &GstPlayer::onRecordValueChanged);
    connect(f_reencoding, &Fact::valueChanged, this, &GstPlayer::onReencodingValueChanged);

    AppSettingFact::loadSettings(this);

    m_reconnectTimer.setInterval(RECONNECT_TIMEOUT);
    m_reconnectTimer.setSingleShot(true);

    m_videoThread.setOverlayCallback(std::bind(&Overlay::drawOverlay, f_overlay, _1));

    ApxApp::instance()->engine()->loadQml("qrc:/streaming/VideoPlugin.qml");
}

GstPlayer::~GstPlayer()
{
    if(m_videoThread.isRunning())
        stop();
}

GstPlayer::ConnectionState GstPlayer::getConnectionState() const
{
    return m_connectionState;
}

void GstPlayer::snapshot() const
{
    QImage image = m_lastFrame.copy();
    f_overlay->drawOverlay(image);

    if(!image.save(getMediaFileName(mtImage)))
        onErrorOccured("Can't save snapshot");
}

QString GstPlayer::getMediaFileName(MediaType type)
{
    QString base, ext;
    if(type == mtImage)
    {
        base = ApxDirs::images().absolutePath();
        ext = "png";
    }
    else if(type == mtVideo)
    {
        base = ApxDirs::video().absolutePath();
        ext = "mkv";
    }

    QString currentDateTime = QDateTime::currentDateTime().toString("dd_mm_yyyyThh_mm_ss_zzz");
    QString filename = QString("%1/%2.%3").arg(base, currentDateTime, ext);
    return filename;
}

void GstPlayer::setConnectionState(GstPlayer::ConnectionState cs)
{
    if(m_connectionState != cs)
    {
        m_connectionState = cs;
        emit connectionStateChanged();
    }
}

void GstPlayer::play()
{
    setConnectionState(STATE_CONNECTING);
    m_videoThread.setUrl(f_uri->value().toString());
    m_videoThread.start();
    m_reconnectTimer.start();
}

void GstPlayer::stop()
{
    m_videoThread.stop();
    if(!m_videoThread.wait(THREAD_STOP_TIMEOUT))
    {
        onErrorOccured("VideoThread stop timeout, try to force stop...");
        m_videoThread.terminate();
    }

    QImage splash(m_lastFrame.size(), QImage::Format_RGB32);
    splash.fill(Qt::black);
    onFrameReceived(splash);

    setConnectionState(STATE_UNCONNECTED);
    m_reconnectTimer.stop();
}

void GstPlayer::onFrameReceived(const QImage &image)
{
    m_reconnectTimer.start();

    if(getConnectionState() == STATE_CONNECTING)
        setConnectionState(STATE_CONNECTED);

    m_lastFrame = image;
    if(m_videoSurface)
    {
        if(image.size() != m_videoSurface->surfaceFormat().frameSize())
            m_videoSurface->stop();

        if(!m_videoSurface->isActive())
            m_videoSurface->start(QVideoSurfaceFormat(image.size(), QVideoFrame::Format_RGB32));

        QVideoFrame frame(image);
        if(!m_videoSurface->present(frame))
            onErrorOccured("Can't present frame on surface");
    }
}

void GstPlayer::onActiveValueChanged()
{
    bool active = f_active->value().toBool();
    if(active)
        play();
    else
        stop();
}

void GstPlayer::onRecordValueChanged()
{
    bool record = f_record->value().toBool();
    m_videoThread.setRecording(record);
}

void GstPlayer::onReencodingValueChanged()
{
    bool reencoding = f_reencoding->value().toBool();
    m_videoThread.setReencoding(reencoding);
}

void GstPlayer::onErrorOccured(const QString &error) const
{
    apxMsgW() << error;
}

void GstPlayer::onReconnectTimerTimeout()
{
    onErrorOccured("Connection timeout");
    stop();
    play();
}
