#include "videothread.h"

#include "gstplayer.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <gst/app/gstappsink.h>
#include <QPainter>

using namespace std::placeholders;

VideoThread::VideoThread()
    : m_stop(false)
    , m_recording(false)
    , m_reencoding(false)
    , m_lowLatency(true)
    , m_loop(nullptr)
{
    setupEnvironment();

    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);
    else
        apxMsgW() << "GST already initialized";
}

QString VideoThread::getUri()
{
    QMutexLocker locker(&m_ioMutex);
    return m_uri;
}

void VideoThread::setUri(const QString &uri)
{
    QMutexLocker locker(&m_ioMutex);
    m_uri = uri;
}

void VideoThread::stop()
{
    m_stop = true;
    if (m_loop)
        g_main_loop_quit(m_loop.get());
}

bool VideoThread::getRecording() const
{
    return m_recording;
}

void VideoThread::setRecording(bool b)
{
    m_recording = b;
}

bool VideoThread::getReencoding() const
{
    return m_reencoding;
}

void VideoThread::setReencoding(bool enable)
{
    m_reencoding = enable;
}

void VideoThread::setOverlayCallback(const StreamContext::OverlayCallback &cb)
{
    QMutexLocker locker(&m_ioMutex);
    m_overlayCallback = cb;
}

void VideoThread::removeOverlayCallback()
{
    QMutexLocker locker(&m_ioMutex);
    m_overlayCallback = [](auto) {};
}

void VideoThread::setLowLatency(bool lowLatency)
{
    m_lowLatency = lowLatency;
}

bool VideoThread::getLowLatency() const
{
    return m_lowLatency;
}

static GstFlowReturn on_new_sample_from_sink(GstElement *appsink, StreamContext *context)
{
    context->onFrameReceived(context, appsink);

    return GST_FLOW_OK;
}

static void on_pad_added(GstElement *el1, GstPad *pad, GstElement *el2)
{
    gchar *name = gst_pad_get_name(pad);
    bool ok = gst_element_link_pads(el1, name, el2, "sink");
    if (!ok)
        apxMsgW() << "Can't link pads";
    g_free(name);
}

static void on_pad_added_urisourcebin(GstElement *el1, GstPad *pad, GstElement *el2)
{
    std::shared_ptr<GstCaps> caps(gst_pad_query_caps(pad, nullptr), &gst_caps_unref);
    GstStructure *capsStruct = gst_caps_get_structure(caps.get(), 0);
    QString mediaType = gst_structure_get_string(capsStruct, "media");
    if (mediaType
        != "audio") //without this, if we link audio stream first, pipeline will be stuck without audio sink
        on_pad_added(el1, pad, el2);
}

static GstPadProbeReturn draw_overlay(GstPad *pad, GstPadProbeInfo *info, StreamContext *context)
{
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    buffer = gst_buffer_make_writable(buffer);

    if (buffer == nullptr)
        return GST_PAD_PROBE_OK;

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        std::shared_ptr<GstCaps> caps(gst_pad_get_current_caps(pad), &gst_caps_unref);
        int width = 0;
        int height = 0;
        if (VideoThread::getFrameSizeFromCaps(caps, width, height)) {
            QImage rgb32Wrapper(map.data, width, height, QImage::Format_RGBX8888);
            context->overlayCallback(rgb32Wrapper);
        }
        gst_buffer_unmap(buffer, &map);
    } else
        apxMsgW() << "Can't map buffer";

    GST_PAD_PROBE_INFO_DATA(info) = buffer;

    return GST_PAD_PROBE_OK;
}

void VideoThread::run()
{
    auto context = std::make_unique<StreamContext>();
    context->reencoding = m_reencoding;

    m_loop.reset(g_main_loop_new(nullptr, false), &g_main_loop_unref);

    //common elements
    context->pipeline.reset(gst_pipeline_new("client"), &gst_object_unref);

    context->capsFilter = gst_element_factory_make("capsfilter", nullptr);
    context->source = createSourceElement(context.get());
    context->parsebin = gst_element_factory_make("parsebin", nullptr);

    context->teeparse = gst_element_factory_make("tee", nullptr);
    context->teeconvert = gst_element_factory_make("tee", nullptr);

    //play elements
    context->playDecodebin = gst_element_factory_make("decodebin", nullptr);
    context->playConverter = gst_element_factory_make("videoconvert", nullptr);
    context->playAppsink = gst_element_factory_make("appsink", nullptr);

    gst_bin_add_many(GST_BIN(context->pipeline.get()),
                     context->source,
                     context->capsFilter,
                     context->parsebin,
                     context->teeparse,
                     context->playDecodebin,
                     context->playConverter,
                     context->teeconvert,
                     context->playAppsink,
                     nullptr);

    context->onFrameReceived = std::bind(&VideoThread::onSampleReceived, this, _1, _2);
    context->isRecordRequested = std::bind(&VideoThread::getRecording, this);

    if (!context->pipeline || !context->source || !context->capsFilter || !context->parsebin
        || !context->teeparse || !context->playDecodebin || !context->playConverter
        || !context->playAppsink) {
        emit errorOccured("Can't create pipeline");
        return;
    }

    //appsink configuring
    g_object_set(G_OBJECT(context->playAppsink),
                 "emit-signals",
                 true,
                 "sync",
                 !m_lowLatency,
                 nullptr);
    g_signal_connect(context->playAppsink,
                     "new-sample",
                     G_CALLBACK(on_new_sample_from_sink),
                     context.get());
    gst_app_sink_set_max_buffers(GST_APP_SINK(context->playAppsink), 0);
    gst_app_sink_set_drop(GST_APP_SINK(context->playAppsink), true);
    g_object_set(context->playAppsink, "caps", getCapsForAppSink().get(), nullptr);

    //pad-cb
    if (GST_IS_BIN(context->source)) {
        g_signal_connect(context->source,
                         "pad-added",
                         G_CALLBACK(on_pad_added_urisourcebin),
                         context->capsFilter);
    } else {
        if (!gst_element_link(context->source, context->capsFilter)) {
            emit errorOccured("Can't link source and capsfilter");
            return;
        }
    }
    g_signal_connect(context->parsebin, "pad-added", G_CALLBACK(on_pad_added), context->teeparse);
    g_signal_connect(context->playDecodebin,
                     "pad-added",
                     G_CALLBACK(on_pad_added),
                     context->playConverter);

    if (!gst_element_link_many(context->capsFilter, context->parsebin, nullptr)) {
        emit errorOccured("Can't link source, caps filter and parsebin");
        return;
    }

    if (!gst_element_link(context->teeparse, context->playDecodebin)) {
        emit errorOccured("Can't link tee and decodebin");
        return;
    }

    if (!gst_element_link_many(context->playConverter,
                               context->teeconvert,
                               context->playAppsink,
                               nullptr)) {
        emit errorOccured("Can't link converter, tee and appsink");
        return;
    }

    if (gst_element_set_state(context->pipeline.get(), GST_STATE_PLAYING)
        == GST_STATE_CHANGE_FAILURE) {
        emit errorOccured("Can't play pipeline");
        return;
    }
    g_main_loop_run(m_loop.get());

    if (context->recording)
        closeWriter(context.get());

    gst_element_set_state(context->pipeline.get(), GST_STATE_NULL);
}

GstElement *VideoThread::createSourceElement(StreamContext *context)
{
    GstElement *result = nullptr;
    if (m_uri.contains("avf://")) {
        QUrl u(m_uri);
        result = gst_element_factory_make("avfvideosrc", nullptr);
        bool ok;
        int index = u.host().remove("index").toInt(&ok);
        if (ok)
            g_object_set(result, "device-index", index, nullptr);

        g_object_set(context->capsFilter, "caps", gst_caps_from_string("video/x-raw"), nullptr);
    } else if (m_uri.contains("tcp://")) {
        QUrl u(m_uri);
        result = gst_element_factory_make("tcpclientsrc", nullptr);
        QString host = u.host();
        int port = u.port();
        g_object_set(result, "host", host.toStdString().c_str(), nullptr);
        g_object_set(result, "port", port, nullptr);

        g_object_set(context->capsFilter,
                     "caps",
                     gst_caps_from_string("video/mpegts,systemstream=true"),
                     nullptr);
    } else if (m_uri.contains("udp://")) {
        QUrl u(m_uri);
        result = gst_element_factory_make("udpsrc", nullptr);
        int port = u.port();
        QString query = u.query();
        g_object_set(result, "port", port, nullptr);

        std::shared_ptr<GstCaps> caps;
        if (query.contains("codec=h265"))
            caps = getCapsForUdpSrc("H265");
        else
            caps = getCapsForUdpSrc("H264");
        g_object_set(result, "caps", caps.get(), nullptr);
    } else {
        result = gst_element_factory_make("urisourcebin", nullptr);
        g_object_set(result, "uri", m_uri.toStdString().c_str(), nullptr);
    }

    return result;
}

void VideoThread::openWriter(StreamContext *context)
{
    QString filename = GstPlayer::getMediaFileName(GstPlayer::mtVideo);

    //create
    context->recQueue = gst_element_factory_make("queue", nullptr);
    if (context->reencoding) {
        context->recConverter = gst_element_factory_make("videoconvert", nullptr);
        context->recEncoder = gst_element_factory_make("x264enc", nullptr);
    }
    context->recMuxer = gst_element_factory_make("matroskamux", nullptr);
    context->recSink = gst_element_factory_make("filesink", nullptr);

    if (!context->recQueue || !context->recMuxer || !context->recSink
        || (context->reencoding && (!context->recEncoder || !context->recConverter))) {
        emit errorOccured("Can't create recording elements");
        return;
    }

    //tune
    g_object_set(G_OBJECT(context->recSink), "location", filename.toStdString().c_str(), nullptr);
    if (context->reencoding) {
        g_object_set(G_OBJECT(context->recEncoder), "speed-preset", 1, nullptr);
        g_object_set(G_OBJECT(context->recEncoder), "tune", 4, nullptr);
    }

    //add
    gst_bin_add_many(GST_BIN(context->pipeline.get()),
                     context->recQueue,
                     context->recMuxer,
                     context->recSink,
                     nullptr);
    if (context->reencoding)
        gst_bin_add_many(GST_BIN(context->pipeline.get()),
                         context->recConverter,
                         context->recEncoder,
                         nullptr);

    //link
    bool linkResult = false;
    if (!context->reencoding)
        linkResult = gst_element_link_many(context->teeparse,
                                           context->recQueue,
                                           context->recMuxer,
                                           context->recSink,
                                           nullptr);
    else
        linkResult = gst_element_link_many(context->teeconvert,
                                           context->recQueue,
                                           context->recConverter,
                                           context->recEncoder,
                                           context->recMuxer,
                                           context->recSink,
                                           nullptr);
    if (!linkResult) {
        emit errorOccured("Can't link elements from record pipeline");
        return;
    }

    //sync
    bool syncResult = gst_element_sync_state_with_parent(context->recQueue)
                      && gst_element_sync_state_with_parent(context->recMuxer)
                      && gst_element_sync_state_with_parent(context->recSink);
    if (context->reencoding) {
        syncResult = syncResult && gst_element_sync_state_with_parent(context->recConverter)
                     && gst_element_sync_state_with_parent(context->recEncoder);
    }
    if (!syncResult) {
        emit errorOccured("Can't sync state of rec elements with parent");
        return;
    }

    //overlay callback
    if (context->reencoding) {
        std::shared_ptr<GstPad> pad(gst_element_get_static_pad(context->recQueue, "src"),
                                    &gst_object_unref);
        gst_pad_add_probe(pad.get(),
                          GST_PAD_PROBE_TYPE_BUFFER,
                          (GstPadProbeCallback) draw_overlay,
                          context,
                          nullptr);
        context->overlayCallback = m_overlayCallback;
    } else
        context->overlayCallback = [](auto) {};

    context->recording = true;
}

void VideoThread::closeWriter(StreamContext *context)
{
    gst_element_send_event(context->recMuxer, gst_event_new_eos());

    //remove
    gst_bin_remove_many(GST_BIN(context->pipeline.get()),
                        context->recQueue,
                        context->recMuxer,
                        context->recSink,
                        nullptr);
    if (context->reencoding)
        gst_bin_remove_many(GST_BIN(context->pipeline.get()),
                            context->recConverter,
                            context->recEncoder,
                            nullptr);

    gst_element_set_state(context->recQueue, GST_STATE_NULL);
    gst_element_set_state(context->recMuxer, GST_STATE_NULL);
    gst_element_set_state(context->recSink, GST_STATE_NULL);

    if (context->reencoding) {
        gst_element_set_state(context->recConverter, GST_STATE_NULL);
        gst_element_set_state(context->recEncoder, GST_STATE_NULL);
    }

    context->recording = false;
}

void VideoThread::onSampleReceived(StreamContext *context, GstElement *appsink)
{
    std::shared_ptr<GstSample> sample(gst_app_sink_pull_sample(GST_APP_SINK(appsink)),
                                      &gst_sample_unref);
    QImage frame = sample2qimage(sample);

    if (!context->recording && context->isRecordRequested())
        openWriter(context);

    if (context->recording && !context->isRecordRequested())
        closeWriter(context);

    emit frameReceived(frame);
}

QImage VideoThread::sample2qimage(const std::shared_ptr<GstSample> &sample)
{
    std::shared_ptr<GstCaps> caps(gst_sample_get_caps(sample.get()), [](auto) {});
    int width = 0;
    int height = 0;
    if (getFrameSizeFromCaps(caps, width, height)) {
        GstBuffer *buffer = gst_sample_get_buffer(sample.get());
        GstMapInfo info;
        gboolean success = gst_buffer_map(buffer, &info, (GstMapFlags) GST_MAP_READ);
        if (success) {
            QImage rgb32Wrapper(info.data, width, height, QImage::Format_RGBX8888);
            QImage rgb32Copy = rgb32Wrapper.copy();
            gst_buffer_unmap(buffer, &info);
            return rgb32Copy.convertToFormat(QImage::Format_RGB32);
        }
    }
    return QImage(width, height, QImage::Format_RGB32);
}

void VideoThread::setupEnvironment()
{
    return;
    QString scannerPath;
    QString pluginsPath;
    QString gioPath;

#ifdef Q_OS_LINUX
    scannerPath = "../lib/x86_64-linux-gnu/gstreamer1.0/gstreamer-1.0";
    pluginsPath = "../lib/x86_64-linux-gnu/gstreamer-1.0";
    gioPath = "../lib/x86_64-linux-gnu/gio/modules";
#endif
#ifdef Q_OS_MAC
    scannerPath = "../Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0";
    pluginsPath = "../Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0";
    gioPath = "../Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules";
#endif

    QDir appDir(QCoreApplication::applicationDirPath());
    QDir scannerDir(appDir);
    QDir pluginsDir(appDir);
    QDir gioDir(appDir);

    if (scannerDir.cd(scannerPath) && scannerDir.exists("gst-plugin-scanner")
        && pluginsDir.cd(pluginsPath) && gioDir.cd(gioPath)) {
        qputenv("GST_PLUGIN_SCANNER", scannerDir.absoluteFilePath("gst-plugin-scanner").toUtf8());
        qputenv("GIO_EXTRA_MODULES", gioDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", pluginsDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_SYSTEM_PATH", pluginsDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_PATH_1_0", pluginsDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_PATH", pluginsDir.absolutePath().toUtf8());
#ifdef Q_OS_LINUX
        pluginsDir.cdUp();
        QDir ld1(pluginsDir);
        pluginsDir.cdUp();
        QDir ld2(pluginsDir);
        qputenv("LD_LIBRARY_PATH",
                QString("%1:%2:$LD_LIBRARY_PATH")
                    .arg(ld1.absolutePath())
                    .arg(ld2.absolutePath())
                    .toUtf8());
#endif
    } else
        qInfo() << "Can't find gstreamer in bundle, try to use system libs"
                << scannerDir.absolutePath();
}

std::shared_ptr<GstCaps> VideoThread::getCapsForAppSink()
{
    auto caps = gst_caps_from_string("video/x-raw,format=RGBx,pixel-aspect-ratio=1/1");
    return std::shared_ptr<GstCaps>(caps, &gst_caps_unref);
}

std::shared_ptr<GstCaps> VideoThread::getCapsForUdpSrc(const std::string &codec)
{
    std::string s = "application/x-rtp,media=video,clock-rate=90000,encoding-name=" + codec;
    auto caps = gst_caps_from_string(s.c_str());
    return std::shared_ptr<GstCaps>(caps, &gst_caps_unref);
}

bool VideoThread::getFrameSizeFromCaps(const std::shared_ptr<GstCaps> &caps, int &width, int &height)
{
    GstStructure *capsStruct = gst_caps_get_structure(caps.get(), 0);
    bool result = gst_structure_get_int(capsStruct, "width", &width)
                  && gst_structure_get_int(capsStruct, "height", &height);
    return result;
}
