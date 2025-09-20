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

// Credits to Pavel Mikhadzionak <pmv@uavos.com>

#pragma once

#include <QVideoSink>
#include <QtCore>
#include <QtMultimedia>

#include "QmlOverlay.h"
#include "videothread.h"

#include <App/AppDirs.h>
#include <Fact/Fact.h>

class Unit;
class GstPlayer : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QVideoSink *videoSink MEMBER m_videoSink)
    Q_PROPERTY(ConnectionState connectionState READ getConnectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(quint64 frameCnt READ frameCnt NOTIFY frameCntChanged)

public:
    static const int THREAD_STOP_TIMEOUT = 500;
    static const int RECONNECT_TIMEOUT = 15000;
    enum ConnectionState { STATE_UNCONNECTED, STATE_CONNECTING, STATE_CONNECTED };
    enum MediaType { mtImage, mtVideo };
    enum SourceType { stUri, stRtsp, stTcp, stUdp, stWebcam };
    enum CodecType { ctH264, ctH265 };

    Q_ENUM(ConnectionState)
    explicit GstPlayer(Fact *parent = nullptr);
    ~GstPlayer() override;

    Fact *f_tune;
    Fact *f_active;
    Fact *f_record;
    Fact *f_reencoding;
    Fact *f_lowLatency;
    Fact *f_viewMode;
    Fact *f_overlay;
    Fact *f_controls;
    Fact *f_tools;

    Fact *f_sourceType;

    Fact *f_uriInput;
    Fact *f_rtspInput;
    Fact *f_rtspTcpForce;
    Fact *f_tcpInput;
    Fact *f_tcpPortInput;
    Fact *f_udpInput;
    Fact *f_udpCodecInput;
    Fact *f_webcamInput;

    ConnectionState getConnectionState() const;
    quint64 frameCnt() const;
    void setFrameCnt(quint64 v);

    Q_INVOKABLE void snapshot() const;

    static QString getMediaFileName(MediaType type);

private:
    QVideoSink *m_videoSink = nullptr;
    VideoThread m_videoThread;
    QImage m_lastFrame;
    ConnectionState m_connectionState = STATE_UNCONNECTED;
    QTimer m_reconnectTimer;
    quint64 m_frameCnt;

    QmlOverlay *overlay{nullptr};

    void setConnectionState(ConnectionState cs);

    void play();
    void stop();

    QString inputToUri();

    QStringList getAvailableWebcams();

    inline static const QDir dir_video{AppDirs::user().absoluteFilePath("Video")};
    inline static const QDir dir_images{AppDirs::user().absoluteFilePath("Images")};

private slots:
    void stopAndPlay();

    void onFrameReceived(const QVideoFrame &frame);
    void onActiveValueChanged();
    void onRecordValueChanged();
    void onSourceTypeChanged();
    void onErrorOccured(const QString &error) const;
    void onReconnectTimerTimeout();

    void unitSelected(Unit *unit);

signals:
    void connectionStateChanged();
    void frameCntChanged();

    void overlayNumbersChanged(); //fwd from qml only
};
