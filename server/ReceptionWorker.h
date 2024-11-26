#include <QObject>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}
#include <unistd.h>

class ReceptionWorker : public QObject {
    Q_OBJECT
public:
    explicit ReceptionWorker(int socket_fd, QObject* parent = nullptr)
    : QObject(parent), socket_fd(socket_fd) {}
    ~ReceptionWorker() {
    if (socket_fd >= 0) {
        close(socket_fd);  
    }
}
    bool receivePacket(int socket_fd, AVPacket* packet) {
        int64_t pts_net, dts_net;
        int32_t stream_index_net, size_net;
        if (recv(socket_fd, &pts_net, sizeof(pts_net), 0) <= 0 ||
            recv(socket_fd, &dts_net, sizeof(dts_net), 0) <= 0 ||
            recv(socket_fd, &stream_index_net, sizeof(stream_index_net), 0) <= 0 ||
            recv(socket_fd, &size_net, sizeof(size_net), 0) <= 0) {
            std::cerr << "Failed to receive packet metadata" << std::endl;
            return false;
        }
        packet->pts = ntohll(pts_net);
        packet->dts = ntohll(dts_net);
        packet->stream_index = ntohl(stream_index_net);
        packet->size = ntohl(size_net);

        packet->data = (uint8_t*)av_malloc(packet->size);
        if (!packet->data) {
            std::cerr << "Failed to allocate memory for packet data" << std::endl;
            return false;
        }

        int total_received = 0;
        while (total_received < packet->size) {
            int received = recv(socket_fd, packet->data + total_received, packet->size - total_received, 0);
            if (received <= 0) {
                std::cerr << "Failed to receive packet data" << std::endl;
                av_free(packet->data);
                return false;
            }
            total_received += received;
        }
        

        return true;
    }

    int64_t ntohll(int64_t value) {
        return (((int64_t)ntohl(value & 0xFFFFFFFF)) << 32) | ntohl(value >> 32);
    }

signals:
    void packetReady(AVPacket packet);
    void errorOccurred(const QString& error);
    void lostConnection();

public slots:
    void getPacket(){
        AVPacket packet;
        while (true) {
            av_init_packet(&packet);
            if (!receivePacket(socket_fd, &packet)) {
                emit lostConnection();
                close(socket_fd);  
                std::cout<<"eclosing the socket on failure"<<std::endl;
                break;
            }
             emit packetReady(packet);           
        }
    }

private:
    int socket_fd;
    

};
