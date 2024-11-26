#include <QThread>
#include <QLabel>
#include <iostream>
#include <vector>
#include "ThreadSafeQueue.h"
#include <QGridLayout>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <QCoreApplication>
#include "ReceptionWorker.h"
#include "DecodingWorker.h"
#include "RenderingWorker.h"
#include "ThreadSafeQueue.h"
#include <QScreen>
#include <QApplication>
#include "AcceptWorker.h"
#include <chrono>
using namespace std::chrono;


class ConnectionWorker : public QObject {
    Q_OBJECT

public:
    ConnectionWorker(QWidget* parentWindow, std::vector<QLabel*> labels, std::pair<int, int> frame_size) :
        window(parentWindow),
        labels(labels),
        frame_size(frame_size) {
            for (int i = 0; i < labels.size(); ++i) {
                availableLabelIndices.push(i);
            }
        }

    ~ConnectionWorker() {
        cleanup();
    }

    void startAcceptingConnections(int server_fd) {
        acceptWorker = new AcceptWorker(server_fd,stopAccepting);
        acceptThread = new QThread();
        acceptWorker->moveToThread(acceptThread);
        QObject::connect(acceptThread, &QThread::started, acceptWorker, &AcceptWorker::startAccepting);
        QObject::connect(acceptWorker, &AcceptWorker::newConnection, this, &ConnectionWorker::setupClientConnection);
        QObject::connect(acceptThread, &QThread::finished, acceptWorker, &QObject::deleteLater);
        QObject::connect(acceptThread, &QThread::finished, acceptThread, &QObject::deleteLater);


        acceptThread->start();

    }

 signals:
    void stopAccept();
public slots:
    void setupClientConnection(int new_socket) {

        if (availableLabelIndices.empty()) {
            close(new_socket);
            if(!stopAccepting.load()){
                acceptWorker->stop();
            }
            std::cout << "All labels are occupied. No more new connections allowed!" << std::endl;
            return;
        }

        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "Codec not found!" << std::endl;
            return;
        }

        AVCodecContext* codec_context = avcodec_alloc_context3(codec);
        if (!codec_context) {
            std::cerr << "Could not allocate codec context!" << std::endl;
            return;
        }

        auto [end_width, end_height] = frame_size;
        codec_context->width = end_width;
        codec_context->height = end_height;

        if (avcodec_open2(codec_context, codec, nullptr) < 0) {
            std::cerr << "Could not open codec!" << std::endl;
            avcodec_free_context(&codec_context);
            return;
        }

        int labelIndex = availableLabelIndices.pop();
        QLabel* label = labels.at(labelIndex);
        ThreadSafeQueue<QImage>* frameQueue = new ThreadSafeQueue<QImage>();
        int targetFPS = 50;

        QThread* processingThread = new QThread();
        ReceptionWorker* receptionWorker = new ReceptionWorker(new_socket);
        DecodingWorker* decodingWorker = new DecodingWorker(codec_context, *frameQueue);
        RenderingWorker* renderingWorker = new RenderingWorker(label, *frameQueue, targetFPS);

        receptionWorker->moveToThread(processingThread);
        decodingWorker->moveToThread(processingThread);

        QObject::connect(processingThread, &QThread::started, receptionWorker, &ReceptionWorker::getPacket);
        QObject::connect(receptionWorker, &ReceptionWorker::packetReady, decodingWorker, &DecodingWorker::getFrame);
        QObject::connect(receptionWorker, &ReceptionWorker::lostConnection, this, 
    [this, labelIndex, renderingWorker, decodingWorker, receptionWorker, processingThread, label, frameQueue, codec_context]() {
        renderingWorker->requestShutdown();
        
        QEventLoop loop;
        QObject::connect(renderingWorker, &RenderingWorker::finished, &loop, &QEventLoop::quit);
        loop.exec();

        processingThread->quit();
        workerThreads.erase(std::remove(workerThreads.begin(), workerThreads.end(), processingThread), workerThreads.end());
        frameQueues.erase(std::remove(frameQueues.begin(), frameQueues.end(), frameQueue), frameQueues.end());
        codecContexts.erase(std::remove(codecContexts.begin(), codecContexts.end(), codec_context), codecContexts.end());
    
        availableLabelIndices.push(labelIndex);

        QObject::connect(processingThread, &QThread::finished, processingThread, &QThread::deleteLater);
        QObject::connect(processingThread, &QThread::finished, receptionWorker, &QThread::deleteLater);
        //QObject::connect(renderingWorker, &QThread::finished, renderingWorker, &QThread::deleteLater);

        if(stopAccepting.load()){
            acceptWorker->start();
        }

        std::cout << "Scheduled cleanup for label index: " << labelIndex << std::endl;
    }
);
        processingThread->start();
        renderingWorker->start();
        workerThreads.push_back(processingThread);
        frameQueues.push_back(frameQueue);
        codecContexts.push_back(codec_context);

    }

private:
    void cleanup() {
        if (acceptThread) {
            acceptWorker->stop();
            acceptThread->quit();
            acceptThread->wait();
            delete acceptWorker;
            delete acceptThread;
        }

        for (auto label : labels) {
            delete label;
        }

        for (auto thread : workerThreads) {
            if (thread->isRunning()) {
                thread->quit();
                thread->wait();
            }
        }

        for (auto queue : frameQueues) {
            delete queue;
        }

        for (auto codecContext : codecContexts) {
            avcodec_free_context(&codecContext);
        }
    }

    QWidget* window;
    QGridLayout* layout;
    QThread* acceptThread = nullptr;
    AcceptWorker* acceptWorker = nullptr;
    std::vector<QLabel*> labels;
    std::vector<QThread*> workerThreads;
    std::vector<ThreadSafeQueue<QImage>*> frameQueues;
    std::vector<AVCodecContext*> codecContexts;
    ThreadSafeQueue<int> availableLabelIndices;
    std::pair<int, int> frame_size;
    std::atomic<bool> stopAccepting{false};  

};