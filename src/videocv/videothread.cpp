#include "videothread.h"

#include <QTime>
#include <QUrl>
#include <iostream>

#define CRITICAL_ERROR(x) \
{ \
    std::cout << x << std::endl; \
    freeMemory(fmt_ctx, opts); \
    emit errorOccured(x); \
    emit stateChanged(QAbstractSocket::UnconnectedState); \
    return; \
    }

static int interruptCallback(void *opaque)
{
    bool *stop = static_cast<bool*>(opaque);
    return *stop;
}

VideoThread::VideoThread(QObject *parent):
    QThread(parent),
    m_stop(false),
    m_inputUrl("udp://127.0.0.1:30010")
{
    av_register_all();
    avformat_network_init();
    avdevice_register_all();

    av_log_set_level(AV_LOG_QUIET);
}

void VideoThread::run()
{
    m_stop = false;
    emit stateChanged(QAbstractSocket::ConnectingState);

    int video_stream_idx = -1;

    AVDictionary *opts = NULL;

    AVFormatContext *fmt_ctx = avformat_alloc_context();

    /* callback */
    fmt_ctx->interrupt_callback.callback = &interruptCallback;
    fmt_ctx->interrupt_callback.opaque = &m_stop;

    /* open input file, and allocate format context */
    m_ioMutex.lock();
    QUrl inputUrl(m_inputUrl);
    m_ioMutex.unlock();
    const QString scheme = inputUrl.scheme();
    if(scheme == "udp" || scheme == "rtp")
        inputUrl.setQuery("overrun_nonfatal=1"); //for some corrupted frames

    const bool realtimeStream = isRealtime(scheme);

    /* silent agreement about transport level and url scheme */
    if(scheme == "rtspt")
    {
        inputUrl.setScheme("rtsp");
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    }
    if(scheme == "rtsp")
        av_dict_set(&opts, "rtsp_transport", "udp", 0);

    /* some tuning for specific protocols */
    if(realtimeStream)
        av_dict_set_int(&opts, "max_delay", 10, 0);

    if(avformat_open_input(&fmt_ctx, inputUrl.toString().toStdString().c_str(), NULL, &opts) < 0)
        CRITICAL_ERROR("Could not open source file");

    /* retrieve stream information */
    if(avformat_find_stream_info(fmt_ctx, NULL) < 0)
        CRITICAL_ERROR("Could not find stream information");

    /* find best stream */
    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_idx < 0)
        CRITICAL_ERROR("Could not find video stream in input file");
    AVStream *video_stream = fmt_ctx->streams[video_stream_idx];
    if(!video_stream)
        CRITICAL_ERROR("Could not find audio or video stream in the input, aborting");

    /* find decoder for the stream */
    AVCodecContext *dec_ctx = video_stream->codec;
    AVCodec *dec = avcodec_find_decoder(dec_ctx->codec_id);
    if(!dec)
        CRITICAL_ERROR("Failed to find codec");

    /* Init the decoders, with or without reference counting */
    av_dict_set(&opts, "refcounted_frames", "0", 0);
    if(avcodec_open2(dec_ctx, dec, &opts) < 0)
        CRITICAL_ERROR("Failed to open codec");

    /* initialize packet, set data to NULL, let the demuxer fill it */
    AVPacket pkt;
    av_init_packet(&pkt);

    /* read frames from the file */
    emit stateChanged(QAbstractSocket::ConnectedState);
    while(!m_stop)
    {
        QTime time;
        time.start();

        if(av_read_frame(fmt_ctx, &pkt) < 0)
            break;
        AVFrame *frame = av_frame_alloc();
        AVPacket orig_pkt = pkt;
        int ret = 0;
        int got_frame = 0;
        if(pkt.stream_index == video_stream_idx)
        {
            /* decode video frame */
            ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, &pkt);
            if(ret < 0)
            {
                std::cout << "Warning: Error when decoding video frame" << std::endl;
                av_frame_free(&frame);
                av_free_packet(&orig_pkt);
                continue;
            }
            if(got_frame)
            {
                m_ioMutex.lock();
                m_frame = avframe2qimage(frame);
                QImage frameCopy = m_frame.copy();
                m_ioMutex.unlock();
                emit frameReceived(frameCopy);
            }
        }
        av_frame_free(&frame);
        av_free_packet(&orig_pkt);

        if(!realtimeStream)
        {
            const double elapsed = time.elapsed();
            const double idealFps = double(video_stream->avg_frame_rate.num) / double(video_stream->avg_frame_rate.den);
            const double realFps = 1000.0 / elapsed;
            if(realFps > idealFps)
                msleep(1000.0 / idealFps - elapsed);
        }
    }
    avcodec_close(dec_ctx);
    avformat_close_input(&fmt_ctx);
    freeMemory(fmt_ctx, opts);
    emit stateChanged(QAbstractSocket::UnconnectedState);
}

bool VideoThread::isRealtime(const QString &scheme)
{
    QStringList realtimeSchemes;
    realtimeSchemes << "http" << "udp" << "rtp" << "rtsp" << "rtspt";
    return realtimeSchemes.contains(scheme);
}

QString VideoThread::inputUrl()
{
    QMutexLocker locker(&m_ioMutex);
    return m_inputUrl;
}

void VideoThread::setInputUrl(const QString &inputUrl)
{
    QMutexLocker locker(&m_ioMutex);
    m_inputUrl = inputUrl;
}

QImage VideoThread::getFrame()
{
    QMutexLocker locker(&m_ioMutex);
    return m_frame;
}

void VideoThread::stop()
{
    m_stop = true;
}

QImage VideoThread::avframe2qimage(AVFrame *frame)
{
    SwsContext *swsContext = sws_getContext(frame->width, frame->height, AVPixelFormat(frame->format),
                                            frame->width, frame->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    AVFrame *frameRgb = av_frame_alloc();
    if(frameRgb == NULL)
        return QImage();
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, frame->width, frame->height);
    uint8_t *buffer = new uint8_t[numBytes];

    avpicture_fill((AVPicture*)frameRgb, buffer, AV_PIX_FMT_RGB32, frame->width, frame->height);

    sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
              frameRgb->data, frameRgb->linesize);

    QImage image(frame->width, frame->height, QImage::Format_RGB32);
    memcpy(image.bits(), frameRgb->data[0], numBytes);
    delete [] buffer;
    av_frame_free(&frameRgb);
    sws_freeContext(swsContext);
    return image;
}

void VideoThread::freeMemory(AVFormatContext *fc, AVDictionary *dict)
{
    avformat_free_context(fc);
    av_dict_free(&dict);
}

