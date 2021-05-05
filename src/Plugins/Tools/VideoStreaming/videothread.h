/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <atomic>
#include <functional>
#include <gst/gst.h>
#include <memory>
#include <QImage>
#include <QMutex>
#include <QThread>
#include <QUrl>

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
    GstElement *source = nullptr;
    GstElement *capsFilter = nullptr;
    GstElement *parsebin = nullptr;
    GstElement *teeparse = nullptr;
    GstElement *teeconvert = nullptr;
    //play elements
    GstElement *playDecodebin = nullptr;
    GstElement *playConverter = nullptr;
    GstElement *playAppsink = nullptr;
    //record elements
    GstElement *recQueue = nullptr;
    GstElement *recConverter = nullptr;
    GstElement *recEncoder = nullptr;
    GstElement *recMuxer = nullptr;
    GstElement *recSink = nullptr;

    bool recording = false;
    bool reencoding = false;

    using OnFrameReceivedLambda = std::function<void(StreamContext *, GstElement *)>;
    using RecordRequstedLambda = std::function<bool()>;
    using OverlayCallback = std::function<void(QImage &)>;
    OnFrameReceivedLambda onFrameReceived = [](auto, auto) {};
    RecordRequstedLambda isRecordRequested = [] { return false; };
    OverlayCallback overlayCallback = [](auto) {};
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

    void setLowLatency(bool lowLatency);
    bool getLowLatency() const;

    void stop();

    static bool getFrameSizeFromCaps(const std::shared_ptr<GstCaps> &caps, int &width, int &height);
    static std::shared_ptr<GstCaps> getCapsForAppSink();
    static std::shared_ptr<GstCaps> getCapsForUdpSrc(const std::string &codec);

protected:
    void run() final;

private:
    QMutex m_ioMutex;
    std::atomic_bool m_stop;
    std::atomic_bool m_recording;
    std::atomic_bool m_reencoding;
    std::atomic_bool m_lowLatency;
    QString m_uri;
    std::shared_ptr<GMainLoop> m_loop;
    StreamContext::OverlayCallback m_overlayCallback;

    GstElement *createSourceElement(StreamContext *context);

    void openWriter(StreamContext *context);
    void closeWriter(StreamContext *context);

    void onSampleReceived(StreamContext *context, GstElement *appsink);

    QImage sample2qimage(const std::shared_ptr<GstSample> &sample);

    void setupEnvironment();

signals:
    void frameReceived(QImage image);
    void errorOccured(QString error);
};
