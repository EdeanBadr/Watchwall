#include <QObject>
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#include <chrono>
#include <thread>
#include <QImage>
#include "ThreadSafeQueue.h"
using namespace std::chrono;

class DecodingWorker : public QObject {
    Q_OBJECT
public:

    explicit DecodingWorker(AVCodecContext* codec_ctx,ThreadSafeQueue<QImage>& frameQueue, QObject* parent = nullptr)
    : QObject(parent), codec_context(codec_ctx), frameQueue(frameQueue) {}

    QImage getQImageFromFrame(const AVFrame* pFrame) {
        SwsContext* img_convert_ctx = sws_getContext(
            pFrame->width,
            pFrame->height,
            (AVPixelFormat)pFrame->format,
            pFrame->width,
            pFrame->height,
            AV_PIX_FMT_RGB24,
            SWS_BICUBIC, NULL, NULL, NULL
        );
        if (!img_convert_ctx) {
            std::cerr << "Failed to create sws context";
            return QImage();
        }

        int rgb_linesizes[8] = {0};
        rgb_linesizes[0] = 3 * pFrame->width;

        unsigned char* rgbData[8];
        int imgBytesSyze = 3 * pFrame->height * pFrame->width;
        rgbData[0] = (unsigned char *)malloc(imgBytesSyze + 64);
        if (!rgbData[0]) {
            std::cerr << "Error allocating buffer for frame conversion";
            sws_freeContext(img_convert_ctx);
            return QImage();
        }

        if (sws_scale(img_convert_ctx,
                      pFrame->data,
                      pFrame->linesize, 0,
                      pFrame->height,
                      rgbData,
                      rgb_linesizes) != pFrame->height) {
            std::cerr << "Error changing frame color range";
            free(rgbData[0]);
            sws_freeContext(img_convert_ctx);
            return QImage();
        }

        QImage image(pFrame->width, pFrame->height, QImage::Format_RGB888);
        for (int y = 0; y < pFrame->height; y++) {
            memcpy(image.scanLine(y), rgbData[0] + y * 3 * pFrame->width, 3 * pFrame->width);
        }

        free(rgbData[0]);
        sws_freeContext(img_convert_ctx);
        return image;
    }

signals:
    void errorOccurred(const QString& error);

public slots:
    void getFrame(const AVPacket& packet) {

            int ret = avcodec_send_packet(codec_context, &packet);
            if (ret < 0) {
                 emit errorOccurred("Error sending packet for decoding.");
                av_packet_unref(const_cast<AVPacket*>(&packet));
                return;
            }
            AVFrame* frame = av_frame_alloc();
            if (avcodec_receive_frame(codec_context, frame) == 0) {
                img = getQImageFromFrame(frame);
                frameQueue.push(img);
            }
            av_frame_free(&frame);
        }
    


private:
    AVCodecContext* codec_context;
    ThreadSafeQueue<QImage>& frameQueue;
    QImage* adress_of_img= new QImage;
    QImage img= *adress_of_img;
};
