#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QMutex>
#include <QImage>
#include <QAbstractSocket>

extern "C"
{
#ifdef UAVOS_PKG
#include <uavos-ffmpeg/libavutil/imgutils.h>
#include <uavos-ffmpeg/libavutil/samplefmt.h>
#include <uavos-ffmpeg/libavutil/timestamp.h>
#include <uavos-ffmpeg/libavformat/avformat.h>
#include <uavos-ffmpeg/libswscale/swscale.h>
#include <uavos-ffmpeg/libavdevice/avdevice.h>
#else
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#endif
}

class VideoThread : public QThread
{
    Q_OBJECT
public:
    explicit VideoThread(QObject *parent = 0);
    QString inputUrl();
    void setInputUrl(const QString &inputUrl);
    QImage getFrame();
    void stop();

protected:
    void run();

private:
    bool m_stop;
    QString m_inputUrl;
    QMutex m_ioMutex;
    QImage m_frame;
    bool isRealtime(const QString &scheme);
    QImage avframe2qimage(AVFrame *frame);
    void freeMemory(AVFormatContext *fc, AVDictionary *dict);

signals:
    void frameReceived(QImage image);
    void errorOccured(QString str);
    void stateChanged(QAbstractSocket::SocketState state);
};

#endif // VIDEOTHREAD_H
