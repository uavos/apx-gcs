#include "gstplayer.h"

#include <QVideoSurfaceFormat>
#include <QtQml>
#include <QCameraInfo>
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

    QSettings *settings = AppSettings::settings();

    setIcon("video");

    f_visible = new AppSettingFact(settings, this, "show_window", tr("Visible"), tr("Show video window"), Bool);
    f_visible->setIcon("check");

    f_tune = new Fact(this, "tune", tr("Tune"), tr("Video stream settings"), Group);
    f_tune->setIcon("tune");

    f_active = new Fact(f_tune, "running", tr("Active"), tr("Receive video stream"), Bool);
    f_active->setIcon("video-input-antenna");

    f_record = new Fact(f_tune, "record", tr("Record"), tr("Save stream to file"), Bool);
    f_record->setIcon("record-rec");

    f_reencoding = new AppSettingFact(settings, f_tune, "reencoding", tr("Reencoding"), tr("Video reencoding"), Bool);
    f_reencoding->setIcon("film");

    f_lowLatency = new AppSettingFact(settings, f_tune, "low_latency", tr("Low latency"),
                                      tr("Disable timestamp synchronization"), Bool, true);
    f_lowLatency->setIcon("speedometer");

    f_sourceType = new AppSettingFact(settings, f_tune, "source_type", tr("Source"), tr("Source type"), Enum, 0);
    f_sourceType->setEnumStrings({"URI", "RTSP", "TCP", "UDP", "Webcam"});

    f_uriInput = new AppSettingFact(settings, f_tune, "uri_input", tr("URI"), tr("rtsp://<..>, file://<..>, etc."), Text);
    f_rtspInput = new AppSettingFact(settings, f_tune, "rtsp_input", tr("URL"), tr("rtsp://<..>"), Text);
    f_rtspTcpForce = new AppSettingFact(settings, f_tune, "rtspforcetcp_input", tr("Force tcp"), "", Bool, false);
    f_tcpInput = new AppSettingFact(settings, f_tune, "tcp_input", tr("IP"), tr("IP address"), Text);
    f_tcpPortInput = new AppSettingFact(settings, f_tune, "tcpport_input", tr("Port"), tr("Port number"), Int);
    f_udpInput = new AppSettingFact(settings, f_tune, "udp_input", tr("Port"), tr("Port number"), Int);
    f_udpCodecInput = new AppSettingFact(settings, f_tune, "udpcodec_input", tr("Codec"), "", Enum, 0);
    f_udpCodecInput->setEnumStrings({"H264", "H265"});
    f_webcamInput = new AppSettingFact(settings, f_tune, "webcam_input", tr("Webcam"), "", Enum);
    f_webcamInput->setEnumStrings(getAvailableWebcams());

    f_overlay = new Overlay(f_tune);
    f_overlay->setIcon("image-plus");

    connect(&m_videoThread, &VideoThread::frameReceived, this, &GstPlayer::onFrameReceived);
    connect(&m_videoThread, &VideoThread::errorOccured, this, &GstPlayer::onErrorOccured);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &GstPlayer::onReconnectTimerTimeout);
    connect(f_active, &Fact::valueChanged, this, &GstPlayer::onActiveValueChanged);
    connect(f_record, &Fact::valueChanged, this, &GstPlayer::onRecordValueChanged);
    connect(f_reencoding, &Fact::valueChanged, this, &GstPlayer::onReencodingValueChanged);
    connect(f_lowLatency, &Fact::valueChanged, this, &GstPlayer::onLowLatencyValueChanged);
    connect(f_sourceType, &Fact::valueChanged, this, &GstPlayer::onSourceTypeChanged);

    m_reconnectTimer.setInterval(RECONNECT_TIMEOUT);
    m_reconnectTimer.setSingleShot(true);

    m_videoThread.setOverlayCallback(std::bind(&Overlay::drawOverlay, f_overlay, _1));

    ApxApp::instance()->engine()->loadQml("qrc:/streaming/VideoPlugin.qml");

    AppSettingFact::loadSettings(this);
    onSourceTypeChanged();
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
    QString uri = inputToUri();
    m_videoThread.setUri(uri);
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

QString GstPlayer::inputToUri()
{
    QString result;
    if(f_sourceType->value().toInt() == stUri)
    {
        result = f_uriInput->value().toString();
    }
    else if(f_sourceType->value().toInt() == stRtsp)
    {
        QString value = f_rtspInput->value().toString();
        if(!value.contains("rtspt://") && f_rtspTcpForce->value().toBool())
        {
            if(value.indexOf("://") == -1)
                result = "rtspt://" + value;
            else
                result = "rtspt://" + value.remove(0, value.indexOf("://") + 3);
        }
        else if(!value.contains("://"))
            result = "rtsp://" + value;
        else
            result = value;
    }
    else if(f_sourceType->value().toInt() == stTcp)
    {
        QString host = f_tcpInput->value().toString();
        int port = f_tcpPortInput->value().toInt();
        result = QString("tcp://%1:%2").arg(host).arg(port);
    }
    else if(f_sourceType->value().toInt() == stUdp)
    {
        int port = f_udpInput->value().toInt();
        QString codec;
        if(f_udpCodecInput->value().toInt() == ctH264)
            codec = "h264";
        else if(f_udpCodecInput->value().toInt() == ctH265)
            codec = "h265";
        QString uri = QString("udp://0.0.0.0:%1?codec=%2").arg(port).arg(codec);
        result = uri;
    }
    else if(f_sourceType->value().toInt() == stWebcam)
    {
#ifdef Q_OS_LINUX
        QString camDescr = f_webcamInput->enumText(f_webcamInput->value().toInt());
        auto cameras = QCameraInfo::availableCameras();
        auto res = std::find_if(cameras.begin(), cameras.end(), [camDescr](auto c){
            return camDescr == c.description();
        });
        if(res == cameras.end())
        {
            onErrorOccured("Can't find webcam");
        }
        else
        {
            result = QString("v4l2://%1").arg(res->deviceName());
        }
#endif
#ifdef Q_OS_MAC
        result = QString("avf://index%1").arg(f_webcamInput->value().toInt());
#endif
    }
    return result;
}

QStringList GstPlayer::getAvailableWebcams()
{
    auto cameras = QCameraInfo::availableCameras();
    QStringList ids;
    std::transform(cameras.begin(), cameras.end(), std::back_inserter(ids), [](auto c){
        return c.description();
    });
    return ids;
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

void GstPlayer::onLowLatencyValueChanged()
{
    bool lowLatency = f_lowLatency->value().toBool();
    m_videoThread.setLowLatency(lowLatency);
}

void GstPlayer::onSourceTypeChanged()
{
    f_uriInput->setVisible(false);
    f_rtspInput->setVisible(false);
    f_rtspTcpForce->setVisible(false);
    f_tcpInput->setVisible(false);
    f_tcpPortInput->setVisible(false);
    f_udpInput->setVisible(false);
    f_udpCodecInput->setVisible(false);
    f_webcamInput->setVisible(false);

    if(f_sourceType->value().toInt() == stUri)
        f_uriInput->setVisible(true);
    else if(f_sourceType->value().toInt() == stRtsp)
    {
        f_rtspInput->setVisible(true);
        f_rtspTcpForce->setVisible(true);
    }
    else if(f_sourceType->value().toInt() == stTcp)
    {
        f_tcpInput->setVisible(true);
        f_tcpPortInput->setVisible(true);
    }
    else if(f_sourceType->value().toInt() == stUdp)
    {
        f_udpInput->setVisible(true);
        f_udpCodecInput->setVisible(true);
    }
    else if(f_sourceType->value().toInt() == stWebcam)
    {
        f_webcamInput->setEnumStrings(getAvailableWebcams());
        f_webcamInput->setVisible(true);
    }
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
