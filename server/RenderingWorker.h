#include <QThread>
#include <QLabel>
#include <QPixmap>
#include <QCoreApplication>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include "ThreadSafeQueue.h"

using namespace std::chrono;
class RenderingWorker : public QThread {
    Q_OBJECT

public:
    RenderingWorker(QLabel* label, ThreadSafeQueue<QImage>& queue, int targetFPS)
        : label(label), frameQueue(queue), targetFPS(targetFPS) {
        shutdownRequested.store(false);
    }
    
    ~RenderingWorker() {
        delete frame;
        delete scaledImage;
    }

    bool requestShutdown() {
        shutdownRequested.store(true);
        return true;


    }

    void run() override {
        auto sleepDuration = std::chrono::milliseconds(1000 / targetFPS);
        const int waitTime = 2; 
        auto lastFrameTime = std::chrono::steady_clock::now();
        
        while (!shutdownRequested.load() || !frameQueue.empty()) {
            if (!frameQueue.empty()) {
                lastFrameTime = std::chrono::steady_clock::now(); 
                *frame = frameQueue.pop();
                *scaledImage = frame->scaled(label->width(), label->height(), Qt::KeepAspectRatio);
                
                QMetaObject::invokeMethod(label, [this, scaledImage=this->scaledImage]() {
                    label->setPixmap(QPixmap::fromImage(*scaledImage));
                }, Qt::QueuedConnection);
                
                if (sleepDuration.count() > 0) {
                    std::this_thread::sleep_for(sleepDuration);
                }
            }
            else if (shutdownRequested.load()) {
                break;
            }
            else {
                auto now = std::chrono::steady_clock::now();
                auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - lastFrameTime).count();
                
                if (elapsedSeconds >= waitTime) {
                    emit finished();
                    break;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        if (shutdownRequested.load()) {
            QMetaObject::invokeMethod(label, [this]() {
                label->setPixmap(QPixmap("../NoStream.png").scaled(label->width(), label->height(), Qt::KeepAspectRatio));
            }, Qt::QueuedConnection);
            
            emit finished();
        }
    }

signals:
    void finished();

private:
    QLabel* label;
    ThreadSafeQueue<QImage>& frameQueue;
    int targetFPS;  
    QImage* frame = new QImage;
    QImage* scaledImage = new QImage;
    std::atomic<bool> shutdownRequested{false};
};