#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QMutex>
#include <QImage>
#include <QUrl>
#include <gst/gst.h>
#include <atomic>
#include <memory>
#include <functional>

/*
 * urisourcebin -> parsebin -> tee -> decodebin -> videoconvert -> tee -> appsink
 *                              │                                   │
 *  ┌───────────────────────────┘                                   │
 *  │                                                               │
 *  queue -> matroskamux -> filesink (without reencoding)           │
 *                                                                  │
 *  ┌───────────────────────────────────────────────────────────────┘
 *  │
 *  queue (probe, draw overlay) -> videoconvert -> x264enc -> matroskamux -> filesink
 *
 */

/* URI examples:
 * avf://index0                     - macos webcam device 0
 * tcp://192.168.1.10:8765          - mpegts h264 stream
 * udp://0.0.0.0:8765?codec=h264    - rtp stream with h264(h265)
 * any other uri that supported by gstreamer
 */

struct StreamContext
{
    std::shared_ptr<GstElement> pipeline;
    //common elements
    GstElement *source          = nullptr;
    GstElement *capsFilter      = nullptr;
    GstElement *parsebin        = nullptr;
    GstElement *teeparse        = nullptr;
    GstElement *teeconvert      = nullptr;
    //play elements
    GstElement *playDecodebin   = nullptr;
    GstElement *playConverter   = nullptr;
    GstElement *playAppsink     = nullptr;
    //record elements
    GstElement *recQueue        = nullptr;
    GstElement *recConverter    = nullptr;
    GstElement *recEncoder      = nullptr;
    GstElement *recMuxer        = nullptr;
    GstElement *recSink         = nullptr;

    bool recording              = false;

    using OnFrameReceivedLambda = std::function<void(StreamContext*, GstElement*)>;
    using RecordRequstedLambda = std::function<bool()>;
    using OverlayCallback = std::function<void(QImage&)>;
    OnFrameReceivedLambda onFrameReceived = [](auto, auto){};
    RecordRequstedLambda isRecordRequested = []{return false;};
    OverlayCallback overlayCallback = [](auto){};
};

class VideoThread : public QThread
{
    Q_OBJECT
public:
    VideoThread();

    QString getUri();
    void setUri(const QString &uri);

    bool getRecording() const;
    void setRecording(bool b);

    bool getReencoding() const;
    void setReencoding(bool enable);

    void setOverlayCallback(const StreamContext::OverlayCallback &cb);
    void removeOverlayCallback();

    void stop();

    static bool getFrameSizeFromCaps(std::shared_ptr<GstCaps> caps, int &width, int &height);
    static std::shared_ptr<GstCaps> getCapsForAppSink();
    static std::shared_ptr<GstCaps> getCapsForUdpSrc(const std::string &codec);

protected:
    void run() final;

private:
    QMutex m_ioMutex;
    std::atomic_bool m_stop;
    std::atomic_bool m_recording;
    std::atomic_bool m_reencoding;
    QString m_uri;
    std::shared_ptr<GMainLoop> m_loop;
    StreamContext::OverlayCallback m_overlayCallback;

    GstElement *createSourceElement(StreamContext *context);

    void openWriter(StreamContext *m_context);
    void closeWriter(StreamContext *m_context);

    void onSampleReceived(StreamContext *m_context, GstElement *appsink);

    QImage sample2qimage(std::shared_ptr<GstSample> sample);

    void setupEnvironment();

signals:
    void frameReceived(QImage image);
    void errorOccured(QString error);
};

#endif // VIDEOTHREAD_H
