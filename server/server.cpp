#include <iostream>
#include <cstring>
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <QApplication>
#include <QLabel>
#include <QGridLayout>
#include <QWidget>
#include <QThread>
#include <QTimer>
#include <vector>
#include <mutex>
#include <QScreen>
#include <QPainter>

#include "ConnectionWorker.h"

#define PORT 8080
#define NO_STREAM_IMAGE "../NoStream.png"

QLabel* createStyledStreamLabel(int width, int height) {
    QLabel* label = new QLabel();
    
    QPixmap placeholderPixmap(NO_STREAM_IMAGE);
    
    QPixmap scaledPixmap = placeholderPixmap.scaled(
        width, 
        height, 
        Qt::KeepAspectRatio
    );
    label->setPixmap(scaledPixmap);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(width, height);
    label->setScaledContents(true);

    label->setStyleSheet(
        "background-color: #2C3E50;"
        "border: 2px solid #34495E;"
        "border-radius: 10px;"
        "color: #BDC3C7;"
        "font-weight: bold;"
    );

    return label;
}

std::pair<int, int> calculateSlotSize(int numLabels, int windowWidth, int windowHeight) {
    if (numLabels <= 0) return {0, 0};

    int rows = std::max(1, static_cast<int>(std::sqrt(numLabels)));
    int cols = std::ceil(static_cast<double>(numLabels) / rows);

    int slotWidth = std::max(200, windowWidth / cols - 20);
    int slotHeight = std::max(150, windowHeight / rows - 20);

    std::cout << "Grid: " << rows << "x" << cols 
              << ", Slot Size: " << slotWidth << "x" << slotHeight << std::endl;

    return {slotWidth, slotHeight};
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <number of streams>" << std::endl;
        return 1;
    }

    int numStreams = std::atoi(argv[1]);
    if (numStreams <= 0) {
        std::cerr << "Number of streams must be a positive integer" << std::endl;
        return 1;
    }

    qRegisterMetaType<AVPacket>("AVPacket");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    QWidget window;
    window.setWindowTitle("Stream Viewer");
    window.setStyleSheet("QWidget { background-color: #ECF0F1; }");

    int windowWidth = 1360, windowHeight = 768;
    window.setFixedSize(windowWidth, windowHeight);

    QGridLayout* layout = new QGridLayout(&window);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);
    window.setLayout(layout);

    int rows = std::max(1, static_cast<int>(std::sqrt(numStreams)));
    int cols = std::ceil(static_cast<double>(numStreams) / rows);

    auto [targetWidth, targetHeight] = calculateSlotSize(numStreams, windowWidth, windowHeight);

    std::vector<QLabel*> labels;
    for (int i = 0; i < numStreams; ++i) {
        QLabel* label = createStyledStreamLabel(targetWidth, targetHeight);
        layout->addWidget(label, i / cols, i % cols);
        labels.push_back(label);
    }

    window.show();

    ConnectionWorker clientManager(&window, labels, {targetWidth, targetHeight});
    clientManager.startAcceptingConnections(server_fd);

    return app.exec();
}