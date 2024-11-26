#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}
#include <unistd.h> 

class Encoder {
public:
    Encoder()
        : codec_context(nullptr), stream(nullptr), codec(nullptr),
          socket_fd(-1) {}

    ~Encoder() {
      if (socket_fd >= 0) {
            std::cout << "Closing socket" << std::endl;
            close(socket_fd); 
        }
    if (codec_context) {
        avcodec_free_context(&codec_context);
    }
    
    }

    bool initialize(int width, int height, int socket_fd){
        this->socket_fd = socket_fd;
        if (!setupCodec(width, height, time_base, framerate)) {
            return false;
        }
        return true;
    }

    bool setupCodec(int width, int height, AVRational time_base, AVRational framerate) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "Codec not found" << std::endl;
            return false;
        }

        codec_context = avcodec_alloc_context3(codec);
        if (!codec_context) {
            std::cerr << "Could not allocate codec context" << std::endl;
            return false;
        }

        codec_context->width = width;
        codec_context->height = height;
        codec_context->time_base = time_base;
        codec_context->framerate = framerate;
        codec_context->gop_size = 10;
        codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

        if (avcodec_open2(codec_context, codec, nullptr) < 0) {
            std::cerr << "Could not open codec" << std::endl;
            return false;
        }

        return true;
    }
    bool writeFrame(AVFrame* frame) {
        if (avcodec_send_frame(codec_context, frame) < 0) {
            return false;
            close(socket_fd);
        }

        while (true) {
            AVPacket packet = {};
            if (avcodec_receive_packet(codec_context, &packet) < 0) {
                break;
                close(socket_fd);
            }
            int64_t pts_net = htonll(packet.pts);
            int64_t dts_net = htonll(packet.dts);
            int32_t stream_index_net = htonl(packet.stream_index);
            int32_t size_net = htonl(packet.size);

            if (send(socket_fd, &pts_net, sizeof(pts_net), 0) < 0 ||
                send(socket_fd, &dts_net, sizeof(dts_net), 0) < 0 ||
                send(socket_fd, &stream_index_net, sizeof(stream_index_net), 0) < 0 ||
                send(socket_fd, &size_net, sizeof(size_net), 0) < 0) {
                std::cerr << "Failed to send packet metadata" << std::endl;
                //close(socket_fd);
                return false;
            }

            if (send(socket_fd, packet.data, packet.size, 0) < 0) {
                std::cerr << "Failed to send packet data" << std::endl;
                //close(socket_fd);
                return false;
            }

            av_packet_unref(&packet);
           // close(socket_fd);
        }

        return true;
    }


    static int64_t htonll(int64_t value) {
        return (((int64_t)htonl(value & 0xFFFFFFFF)) << 32) | htonl(value >> 32);
    }

private:
    AVCodecContext* codec_context;
    AVStream* stream;
    const AVCodec* codec;
    int socket_fd;
    AVRational time_base = {1, 15};
    AVRational framerate = {15, 1};
};
