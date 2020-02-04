#include "gstplayer.h"

#include <Vehicles/Vehicle.h>
#include <Vehicles/VehicleMandala.h>
#include <Vehicles/Vehicles.h>

#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <QCameraInfo>
#include <QVideoSurfaceFormat>
#include <QtQml>

using namespace std::placeholders;

GstPlayer::GstPlayer(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Video"),
           tr("Camera link and streaming"),
           Group,
           "video")
    , m_frameCnt(0)
{
    if (!AppDirs::images().exists())
        AppDirs::images().mkpath(AppDirs::images().absolutePath());
    if (!AppDirs::video().exists())
        AppDirs::video().mkpath(AppDirs::video().absolutePath());

    qmlRegisterType<GstPlayer>("GstPlayer", 1, 0, "GstPlayer");

    f_tune = new Fact(this, "tune", tr("Tune"), tr("Video stream settings"), Group, "tune");

    f_sourceType = new Fact(f_tune,
                            "source_type",
                            tr("Source"),
                            tr("Source type"),
                            Enum | PersistentValue);
    f_sourceType->setEnumStrings({"URI", "RTSP", "TCP", "UDP", "Webcam"});

    f_rtspInput = new Fact(f_tune,
                           "rtsp_input",
                           tr("URL"),
                           tr("rtsp://<..>"),
                           Text | PersistentValue);
    f_rtspTcpForce = new Fact(f_tune,
                              "rtspforcetcp_input",
                              tr("Force tcp"),
                              "",
                              Bool | PersistentValue);
    f_tcpInput = new Fact(f_tune, "tcp_input", tr("IP"), tr("IP address"), Text | PersistentValue);
    f_tcpPortInput = new Fact(f_tune,
                              "tcpport_input",
                              tr("Port"),
                              tr("Port number"),
                              Int | PersistentValue);
    f_udpInput = new Fact(f_tune, "udp_input", tr("Port"), tr("Port number"), Int | PersistentValue);
    f_udpCodecInput = new Fact(f_tune, "udpcodec_input", tr("Codec"), "", Enum | PersistentValue);
    f_udpCodecInput->setEnumStrings({"H264", "H265"});
    f_webcamInput = new Fact(f_tune, "webcam_input", tr("Webcam"), "", Enum | PersistentValue);
    f_webcamInput->setEnumStrings(getAvailableWebcams());

    f_uriInput = new Fact(f_tune,
                          "uri_input",
                          tr("URI"),
                          tr("rtsp://<..>, file://<..>, etc."),
                          Text | PersistentValue);
    f_uriInput->setDefaultValue("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov");

    f_active = new Fact(f_tune,
                        "running",
                        tr("Active"),
                        tr("Receive video stream"),
                        Bool,
                        "video-input-antenna");

    f_record
        = new Fact(f_tune, "record", tr("Record"), tr("Save stream to file"), Bool, "record-rec");

    f_reencoding = new Fact(f_tune,
                            "reencoding",
                            tr("Record overlay"),
                            tr("Video reencoding"),
                            Bool | PersistentValue,
                            "film");

    f_lowLatency = new Fact(f_tune,
                            "low_latency",
                            tr("Low latency"),
                            tr("Disable timestamp synchronization"),
                            Bool | PersistentValue,
                            "speedometer");
    f_lowLatency->setDefaultValue(true);

    f_viewMode = new Fact(f_tune,
                          "view_mode",
                          tr("Fit to view"),
                          tr("Scale video to fit view"),
                          Enum | PersistentValue,
                          "fit-to-page-outline");
    f_viewMode->setEnumStrings({"Fit", "Scale", "Full"});
    f_viewMode->setDefaultValue("Full");

    // overlay
    f_overlay = new Fact(f_tune,
                         "overlay",
                         tr("Overlay"),
                         tr("Show additional info on video"),
                         Group,
                         "image-plus");
    Fact *f;

    f = new Fact(f_overlay, "aim", tr("Aim"), "", Enum | PersistentValue, "crosshairs");
    f->setEnumStrings({"none", "crosshair", "rectangle"});
    f->setDefaultValue("rectangle");

    f = new Fact(f_overlay,
                 "gimbal_yaw_var",
                 tr("Gimbal yaw"),
                 tr("Gimbal yaw position variable"),
                 Mandala | PersistentValue);
    f->setDefaultValue("est.cam.yaw");

    f = new Fact(f_overlay,
                 "gimbal_pitch_var",
                 tr("Gimbal pitch"),
                 tr("Gimbal pitch position variable"),
                 Mandala | PersistentValue);
    f->setDefaultValue("est.cam.pitch");

    // controls
    f_controls = new Fact(f_tune,
                          "controls",
                          tr("Controls"),
                          tr("Enable cam controls"),
                          Group | Bool | PersistentValue,
                          "camera-control");
    f_controls->setDefaultValue("true");

    f = new Fact(f_controls,
                 "control_x",
                 tr("Control X"),
                 tr("Horizontal axis control variable"),
                 Mandala | PersistentValue);
    f->setDefaultValue("cmd.gimbal.yaw");
    f = new Fact(f_controls,
                 "control_sx",
                 tr("Span X"),
                 tr("Horizontal axis span"),
                 Int | PersistentValue);
    f->setDefaultValue(180);

    f = new Fact(f_controls,
                 "control_y",
                 tr("Control Y"),
                 tr("Vertical axis control variable"),
                 Mandala | PersistentValue);
    f->setDefaultValue("cmd.gimbal.pitch");
    f = new Fact(f_controls,
                 "control_sy",
                 tr("Span Y"),
                 tr("Vertical axis span"),
                 Int | PersistentValue);
    f->setDefaultValue(90);

    f = new Fact(f_controls,
                 "rev_y",
                 tr("Reverse Y"),
                 tr("Invert vertical control"),
                 Bool | PersistentValue);

    f = new Fact(f_controls,
                 "rev_zoom",
                 tr("Reverse zoom"),
                 tr("Invert zoom by mouse wheel"),
                 Bool | PersistentValue);

    // controls menu
    f_tools = new Fact(f_tune, "tools", tr("Tools"), tr("Camera tools"), Action, "camera-plus");
    new Fact(f_tools, "ctr.pwr.payload");
    new Fact(f_tools, "cmd.gimbal.mode");
    new Fact(f_tools, "cmd.gimbal.broll");
    new Fact(f_tools, "cmd.gimbal.bpitch");
    new Fact(f_tools, "cmd.gimbal.byaw");
    new Fact(f_tools, "cmd.cam.ch");
    new Fact(f_tools, "cmd.cam.zoom");
    new Fact(f_tools, "cmd.cam.focus");
    new Fact(f_tools, "cmd.cam.pf");
    new Fact(f_tools, "cmd.cam.nir");
    new Fact(f_tools, "cmd.cam.fm");
    new Fact(f_tools, "cmd.cam.ft");
    new Fact(f_tools, "cmd.cam.range");
    new Fact(f_tools, "cmd.cam.shot");
    new Fact(f_tools, "cmd.cam.dshot");
    new Fact(f_tools, "cmd.cam.tshot");
    new Fact(f_tools, "ctr.cam.rec");
    new Fact(f_tools, "ctr.cam.shot");
    new Fact(f_tools, "ctr.cam.arm");
    new Fact(f_tools, "ctr.cam.zin");
    new Fact(f_tools, "ctr.cam.zout");
    new Fact(f_tools, "ctr.cam.aux");
    connect(Vehicles::instance(), &Vehicles::vehicleSelected, this, &GstPlayer::vehicleSelected);
    vehicleSelected(Vehicles::instance()->current());

    connect(&m_videoThread, &VideoThread::frameReceived, this, &GstPlayer::onFrameReceived);
    connect(&m_videoThread, &VideoThread::errorOccured, this, &GstPlayer::onErrorOccured);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &GstPlayer::onReconnectTimerTimeout);
    connect(f_active, &Fact::valueChanged, this, &GstPlayer::onActiveValueChanged);
    connect(f_record, &Fact::valueChanged, this, &GstPlayer::onRecordValueChanged);
    connect(f_sourceType, &Fact::valueChanged, this, &GstPlayer::onSourceTypeChanged);

    m_reconnectTimer.setInterval(RECONNECT_TIMEOUT);
    m_reconnectTimer.setSingleShot(true);

    overlay = new QmlOverlay(this);
    m_videoThread.setOverlayCallback(std::bind(&QmlOverlay::cb_drawOverlay, overlay, _1));

    loadQml(QString("qrc:/%1/VideoPlugin.qml").arg(PLUGIN_NAME));

    onSourceTypeChanged();
}

GstPlayer::~GstPlayer()
{
    if (m_videoThread.isRunning())
        stop();
}

void GstPlayer::vehicleSelected(Vehicle *vehicle)
{
    MandalaTree *m = vehicle->f_mandalatree;
    for (int i = 0; i < f_tools->size(); ++i) {
        Fact *f = f_tools->child(i);
        f->bind(m->fact(f->name().replace('_', '.')));
    }
}

GstPlayer::ConnectionState GstPlayer::getConnectionState() const
{
    return m_connectionState;
}

void GstPlayer::snapshot() const
{
    QImage image = m_lastFrame.copy();
    QImage img = overlay->getSnapshotOverlay(image.size());
    if (!img.isNull()) {
        QPainter painter(&image);
        painter.drawImage(QPoint(0, 0), img);
    }

    if (!image.save(getMediaFileName(mtImage)))
        onErrorOccured("Can't save snapshot");
}

QString GstPlayer::getMediaFileName(MediaType type)
{
    QString base;
    QString ext;
    if (type == mtImage) {
        base = AppDirs::images().absolutePath();
        ext = "png";
    } else if (type == mtVideo) {
        base = AppDirs::video().absolutePath();
        ext = "mkv";
    }

    QString currentDateTime = QDateTime::currentDateTime().toString("dd_mm_yyyyThh_mm_ss_zzz");
    QString filename = QString("%1/%2.%3").arg(base, currentDateTime, ext);
    return filename;
}

void GstPlayer::setConnectionState(GstPlayer::ConnectionState cs)
{
    if (m_connectionState != cs) {
        m_connectionState = cs;
        emit connectionStateChanged();
    }
}

void GstPlayer::play()
{
    setFrameCnt(0);
    setConnectionState(STATE_CONNECTING);
    QString uri = inputToUri();
    m_videoThread.setUri(uri);
    m_videoThread.setLowLatency(f_lowLatency->value().toBool());
    m_videoThread.setReencoding(f_reencoding->value().toBool());
    m_videoThread.start();
    m_reconnectTimer.start();
}

void GstPlayer::stop()
{
    m_videoThread.stop();
    if (!m_videoThread.wait(THREAD_STOP_TIMEOUT)) {
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
    if (f_sourceType->value().toInt() == stUri) {
        result = f_uriInput->value().toString();
    } else if (f_sourceType->value().toInt() == stRtsp) {
        QString value = f_rtspInput->value().toString();
        if (!value.contains("rtspt://") && f_rtspTcpForce->value().toBool()) {
            if (value.indexOf("://") == -1)
                result = "rtspt://" + value;
            else
                result = "rtspt://" + value.remove(0, value.indexOf("://") + 3);
        } else if (!value.contains("://"))
            result = "rtsp://" + value;
        else
            result = value;
    } else if (f_sourceType->value().toInt() == stTcp) {
        QString host = f_tcpInput->value().toString();
        int port = f_tcpPortInput->value().toInt();
        result = QString("tcp://%1:%2").arg(host).arg(port);
    } else if (f_sourceType->value().toInt() == stUdp) {
        int port = f_udpInput->value().toInt();
        QString codec;
        if (f_udpCodecInput->value().toInt() == ctH264)
            codec = "h264";
        else if (f_udpCodecInput->value().toInt() == ctH265)
            codec = "h265";
        QString uri = QString("udp://0.0.0.0:%1?codec=%2").arg(port).arg(codec);
        result = uri;
    } else if (f_sourceType->value().toInt() == stWebcam) {
#ifdef Q_OS_LINUX
        QString camDescr = f_webcamInput->enumText(f_webcamInput->value().toInt());
        auto cameras = QCameraInfo::availableCameras();
        auto res = std::find_if(cameras.begin(), cameras.end(), [camDescr](auto c) {
            return camDescr == c.description();
        });
        if (res == cameras.end()) {
            onErrorOccured("Can't find webcam");
        } else {
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
    std::transform(cameras.begin(), cameras.end(), std::back_inserter(ids), [](auto c) {
        return c.description();
    });
    return ids;
}

void GstPlayer::stopAndPlay()
{
    stop();
    play();
}

void GstPlayer::onFrameReceived(const QImage &image)
{
    m_reconnectTimer.start();

    if (getConnectionState() == STATE_CONNECTING)
        setConnectionState(STATE_CONNECTED);

    setFrameCnt(frameCnt() + 1);
    m_lastFrame = image;
    if (m_videoSurface) {
        if (image.size() != m_videoSurface->surfaceFormat().frameSize())
            m_videoSurface->stop();

        if (!m_videoSurface->isActive())
            m_videoSurface->start(QVideoSurfaceFormat(image.size(), QVideoFrame::Format_RGB32));

        QVideoFrame frame(image);
        if (!m_videoSurface->present(frame))
            onErrorOccured("Can't present frame on surface");
    }
}

void GstPlayer::onActiveValueChanged()
{
    bool active = f_active->value().toBool();
    if (active)
        play();
    else
        stop();
}

void GstPlayer::onRecordValueChanged()
{
    bool record = f_record->value().toBool();
    m_videoThread.setRecording(record);
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

    if (f_sourceType->value().toInt() == stUri)
        f_uriInput->setVisible(true);
    else if (f_sourceType->value().toInt() == stRtsp) {
        f_rtspInput->setVisible(true);
        f_rtspTcpForce->setVisible(true);
    } else if (f_sourceType->value().toInt() == stTcp) {
        f_tcpInput->setVisible(true);
        f_tcpPortInput->setVisible(true);
    } else if (f_sourceType->value().toInt() == stUdp) {
        f_udpInput->setVisible(true);
        f_udpCodecInput->setVisible(true);
    } else if (f_sourceType->value().toInt() == stWebcam) {
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

quint64 GstPlayer::frameCnt() const
{
    return m_frameCnt;
}
void GstPlayer::setFrameCnt(quint64 v)
{
    if (m_frameCnt == v)
        return;
    m_frameCnt = v;
    emit frameCntChanged();
}
