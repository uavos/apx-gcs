#include "videothread.h"

#include <gst/app/gstappsink.h>
#include <QPainter>
#include "ApxLog.h"
#include "ApxDirs.h"
#include "gstplayer.h"

using namespace std::placeholders;

VideoThread::VideoThread():
    m_stop(false),
    m_recording(false),
    m_reencoding(false),
    m_loop(nullptr),
    m_avfWorkaround(false)
{
    QString currentDir = QCoreApplication::applicationDirPath();

    setupEnvironment();

    if(!gst_is_initialized())
        gst_init(nullptr, nullptr);
}

QString VideoThread::getUrl()
{
    QMutexLocker locker(&m_ioMutex);
    return m_url;
}

void VideoThread::setUrl(const QString &url)
{
    QMutexLocker locker(&m_ioMutex);
    m_url = url;
    m_avfWorkaround = false;

    QUrl u(url);
    if(u.scheme() == "avf")
    {
        m_avfWorkaround = true;
        m_url = u.path().remove("/index");
    }
}

void VideoThread::stop()
{
    m_stop = true;
    if(m_loop)
        g_main_loop_quit(m_loop);
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
    m_overlayCallback = [](auto){};
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
    if(!ok)
        apxMsgW() << "Can't link pads";
    g_free(name);
}

static void on_pad_added_urisourcebin(GstElement *el1, GstPad *pad, GstElement *el2)
{
    std::shared_ptr<GstCaps> caps(gst_pad_query_caps(pad, nullptr), &gst_caps_unref);
    GstStructure *capsStruct = gst_caps_get_structure(caps.get(), 0);
    QString mediaType = gst_structure_get_string(capsStruct, "media");
    if(mediaType != "audio")    //without this, if we link audio stream first, pipeline will be stuck without audio sink
        on_pad_added(el1, pad, el2);
}

static GstPadProbeReturn draw_overlay(GstPad *pad, GstPadProbeInfo *info, StreamContext *context)
{
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    buffer = gst_buffer_make_writable(buffer);

    if(buffer == nullptr)
        return GST_PAD_PROBE_OK;

    GstMapInfo map;
    if(gst_buffer_map(buffer, &map, GST_MAP_WRITE))
    {
        std::shared_ptr<GstCaps> caps(gst_pad_get_current_caps(pad), &gst_caps_unref);
        int width = 0;
        int height = 0;
        if(VideoThread::getFrameSizeFromCaps(caps, width, height))
        {
            QImage rgb32Wrapper(map.data, width, height, QImage::Format_RGBX8888);
            context->overlayCallback(rgb32Wrapper);
        }
        gst_buffer_unmap(buffer, &map);
    }
    else
        apxMsgW() << "Can't map buffer";

    GST_PAD_PROBE_INFO_DATA(info) = buffer;

    return GST_PAD_PROBE_OK;
}

void VideoThread::run()
{
    QString url = getUrl();

    m_context = std::make_unique<StreamContext>();

    m_loop = g_main_loop_new(nullptr, false);

    //common elements
    m_context->pipeline = gst_pipeline_new("client");

    if(m_avfWorkaround)
        m_context->source = gst_element_factory_make("avfvideosrc", nullptr);
    else
        m_context->source = gst_element_factory_make("urisourcebin", nullptr);
    m_context->capsFilter = gst_element_factory_make("capsfilter", nullptr);
    m_context->parsebin = gst_element_factory_make("parsebin", nullptr);

    m_context->teeparse = gst_element_factory_make("tee", nullptr);
    m_context->teeconvert = gst_element_factory_make("tee", nullptr);

    //play elements
    m_context->playDecodebin = gst_element_factory_make("decodebin", nullptr);
    m_context->playConverter = gst_element_factory_make("videoconvert", nullptr);
    m_context->playAppsink = gst_element_factory_make("appsink", nullptr);

    gst_bin_add_many(GST_BIN(m_context->pipeline), m_context->source, m_context->capsFilter, m_context->parsebin,
                     m_context->teeparse, m_context->playDecodebin, m_context->playConverter,
                     m_context->teeconvert, m_context->playAppsink, nullptr);

    m_context->onFrameReceived = std::bind(&VideoThread::onSampleReceived, this, _1, _2);
    m_context->isRecordRequested = std::bind(&VideoThread::getRecording, this);

    if(!m_context->pipeline || !m_context->source || !m_context->capsFilter || !m_context->parsebin || !m_context->teeparse ||
            !m_context->playDecodebin || !m_context->playConverter || !m_context->playAppsink)
    {
        emit errorOccured("Can't create pipeline");
        return;
    }

    //source configuring
    if(m_avfWorkaround)
    {
        bool ok;
        int index = m_url.toInt(&ok);
        if(!ok)
        {
            emit errorOccured("Unknown device " + m_url);
            return;
        }
        g_object_set(m_context->source, "device-index", index, nullptr);
        g_object_set(m_context->capsFilter, "caps", gst_caps_from_string("video/x-raw"), nullptr);
    }
    else
        g_object_set(m_context->source, "uri", url.toStdString().data(), nullptr);

    //appsink configuring
    g_object_set(G_OBJECT(m_context->playAppsink), "emit-signals", true, "sync", true, nullptr);
    g_signal_connect(m_context->playAppsink, "new-sample", G_CALLBACK(on_new_sample_from_sink), m_context.get());
    gst_app_sink_set_max_buffers(GST_APP_SINK(m_context->playAppsink), 1);
    gst_app_sink_set_drop(GST_APP_SINK(m_context->playAppsink), true);
    g_object_set(m_context->playAppsink, "caps", getCapsForAppSink(), nullptr);

    //pad-cb
    if(m_avfWorkaround)
        gst_element_link(m_context->source, m_context->capsFilter);
    else
        g_signal_connect(m_context->source, "pad-added", G_CALLBACK(on_pad_added_urisourcebin), m_context->capsFilter);
    g_signal_connect(m_context->parsebin, "pad-added", G_CALLBACK(on_pad_added), m_context->teeparse);
    g_signal_connect(m_context->playDecodebin, "pad-added", G_CALLBACK(on_pad_added), m_context->playConverter);

    if(!gst_element_link_many(m_context->capsFilter, m_context->parsebin, nullptr))
    {
        emit errorOccured("Can't link source, caps filter and parsebin");
        return;
    }

    if(!gst_element_link(m_context->teeparse, m_context->playDecodebin))
    {
        emit errorOccured("Can't link tee and decodebin");
        return;
    }

    if(!gst_element_link_many(m_context->playConverter, m_context->teeconvert, m_context->playAppsink, nullptr))
    {
        emit errorOccured("Can't link converter, tee and appsink");
        return;
    }

    if(gst_element_set_state(m_context->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        emit errorOccured("Can't play pipeline");
        return;
    }
    g_main_loop_run(m_loop);

    gst_element_set_state(m_context->pipeline, GST_STATE_NULL);

    //cleanup
    gst_object_unref(m_context->pipeline);
    g_main_loop_unref(m_loop);
    m_loop = nullptr;
}

void VideoThread::openWriter(StreamContext *context)
{
    QString filename = GstPlayer::getMediaFileName(GstPlayer::mtVideo);

    //create
    context->recQueue = gst_element_factory_make("queue", nullptr);
    if(m_reencoding)
    {
        context->recConverter = gst_element_factory_make("videoconvert", nullptr);
        context->recEncoder = gst_element_factory_make("x264enc", nullptr);
    }
    context->recMuxer = gst_element_factory_make("matroskamux", nullptr);
    context->recSink = gst_element_factory_make("filesink", nullptr);

    if(!context->recQueue || !context->recMuxer || !context->recSink ||
            (m_reencoding && (!context->recEncoder || !context->recConverter)))
    {
        emit errorOccured("Can't create recording elements");
        return;
    }

    //tune
    g_object_set(G_OBJECT(context->recSink), "location", filename.toStdString().c_str(), nullptr);
    g_object_set(G_OBJECT(context->recMuxer), "streamable", true, nullptr);
    if(m_reencoding)
    {
        g_object_set(G_OBJECT(context->recEncoder), "speed-preset", 1, nullptr);
        g_object_set(G_OBJECT(context->recEncoder), "tune", 4, nullptr);
    }

    //add
    gst_bin_add_many(GST_BIN(context->pipeline), context->recQueue, context->recMuxer, context->recSink, nullptr);
    if(m_reencoding)
        gst_bin_add_many(GST_BIN(context->pipeline), context->recConverter, context->recEncoder, nullptr);

    //link
    bool linkResult = false;
    if(!m_reencoding)
        linkResult = gst_element_link_many(context->teeparse, context->recQueue,
                                           context->recMuxer, context->recSink, nullptr);
    else
        linkResult = gst_element_link_many(context->teeconvert, context->recQueue, context->recConverter,
                                           context->recEncoder, context->recMuxer, context->recSink, nullptr);
    if(!linkResult)
    {
        emit errorOccured("Can't link elements from record pipeline");
        return;
    }

    //sync
    bool syncResult = gst_element_sync_state_with_parent(context->recQueue) &&
            gst_element_sync_state_with_parent(context->recMuxer) &&
            gst_element_sync_state_with_parent(context->recSink);
    if(m_reencoding)
    {
        syncResult = syncResult && gst_element_sync_state_with_parent(context->recConverter) &&
                gst_element_sync_state_with_parent(context->recEncoder);
    }
    if(!syncResult)
    {
        emit errorOccured("Can't sync state of rec elements with parent");
        return;
    }

    //overlay callback
    if(m_reencoding)
    {
        std::shared_ptr<GstPad> pad(gst_element_get_static_pad(context->recQueue, "src"), &gst_object_unref);
        gst_pad_add_probe(pad.get(), GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)draw_overlay, context, nullptr);
        context->overlayCallback = m_overlayCallback;
    }
    else
        context->overlayCallback = [](auto){};

    context->recording = true;
}

void VideoThread::closeWriter(StreamContext *context)
{
    //unlink
    if(!m_reencoding)
        gst_element_unlink_many(context->teeparse, context->recQueue, context->recMuxer, context->recSink, nullptr);
    else
        gst_element_unlink_many(context->teeconvert, context->recQueue, context->recConverter,
                                context->recEncoder, context->recMuxer, context->recSink, nullptr);

    //remove
    gst_bin_remove_many(GST_BIN(context->pipeline), context->recQueue, context->recMuxer, context->recSink, nullptr);
    if(m_reencoding)
        gst_bin_remove_many(GST_BIN(context->pipeline), context->recConverter, context->recEncoder, nullptr);

    //unref
    gst_object_unref(context->recQueue);
    gst_object_unref(context->recMuxer);
    gst_object_unref(context->recSink);
    if(m_reencoding)
    {
        gst_object_unref(context->recConverter);
        gst_object_unref(context->recEncoder);
    }

    context->recording = false;
}

void VideoThread::onSampleReceived(StreamContext *context, GstElement *appsink)
{
    std::shared_ptr<GstSample> sample(gst_app_sink_pull_sample(GST_APP_SINK(appsink)), &gst_sample_unref);
    QImage frame = sample2qimage(sample);

    if(!context->recording && context->isRecordRequested())
        openWriter(context);

    if(context->recording && !context->isRecordRequested())
        closeWriter(context);

    emit frameReceived(frame);
}

QImage VideoThread::sample2qimage(std::shared_ptr<GstSample> sample)
{
    std::shared_ptr<GstCaps> caps(gst_sample_get_caps(sample.get()), [](auto){});
    int width = 0;
    int height = 0;
    if(getFrameSizeFromCaps(caps, width, height))
    {
        GstBuffer *buffer = gst_sample_get_buffer(sample.get());
        GstMapInfo info;
        gboolean success = gst_buffer_map(buffer, &info,(GstMapFlags)GST_MAP_READ);
        if(success)
        {
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
    QString scannerPath, pluginsPath, gioPath;

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
    QDir scannerDir(appDir), pluginsDir(appDir), gioDir(appDir);

    if(scannerDir.cd(scannerPath) && scannerDir.exists("gst-plugin-scanner")
            && pluginsDir.cd(pluginsPath) && gioDir.cd(gioPath))
    {
        qputenv("GST_PLUGIN_SCANNER", scannerDir.absoluteFilePath("gst-plugin-scanner").toUtf8());
        qputenv("GIO_EXTRA_MODULES", gioDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", pluginsDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_SYSTEM_PATH", pluginsDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_PATH_1_0", pluginsDir.absolutePath().toUtf8());
        qputenv("GST_PLUGIN_PATH", pluginsDir.absolutePath().toUtf8());
    }
    else
        qInfo() << "Can't find gstreamer in bundle, try to use system libs";
}

GstCaps *VideoThread::getCapsForAppSink()
{
    return gst_caps_from_string("video/x-raw,format=RGBx,pixel-aspect-ratio=1/1");
}

bool VideoThread::getFrameSizeFromCaps(std::shared_ptr<GstCaps> caps, int &width, int &height)
{
    GstStructure *capsStruct = gst_caps_get_structure(caps.get(), 0);
    bool result = gst_structure_get_int(capsStruct, "width", &width) &&
            gst_structure_get_int(capsStruct, "height", &height);
    return result;
}
