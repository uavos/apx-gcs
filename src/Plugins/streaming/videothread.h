#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QMutex>
#include <QImage>
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

struct StreamContext
{
    GstElement *pipeline        = nullptr;
    //common elements
    GstElement *urisourcebin    = nullptr;
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

    QString getUrl();
    void setUrl(const QString &getUrl);

    bool getRecording() const;
    void setRecording(bool b);

    bool getReencoding() const;
    void setReencoding(bool enable);

    void setOverlayCallback(const StreamContext::OverlayCallback &cb);
    void removeOverlayCallback();

    void stop();

    static bool getFrameSizeFromCaps(std::shared_ptr<GstCaps> caps, int &width, int &height);
    static GstCaps* getCapsForAppSink();

protected:
    void run() final;

private:
    QMutex m_ioMutex;
    std::atomic_bool m_stop;
    std::atomic_bool m_recording;
    std::atomic_bool m_reencoding;
    QString m_url;
    GMainLoop *m_loop = nullptr;
    StreamContext::OverlayCallback m_overlayCallback;
    std::unique_ptr<StreamContext> m_context;

    void openWriter(StreamContext *m_context);
    void closeWriter(StreamContext *m_context);

    void onSampleReceived(StreamContext *m_context, GstElement *appsink);

    QImage sample2qimage(std::shared_ptr<GstSample> sample);

signals:
    void frameReceived(QImage image);
    void errorOccured(QString error);
};

#endif // VIDEOTHREAD_H
