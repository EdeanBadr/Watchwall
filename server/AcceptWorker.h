#include <QThread>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <atomic>

class AcceptWorker : public QObject {
    Q_OBJECT

public:
    AcceptWorker(int server_fd, std::atomic<bool>& stopAccepting) : server_fd(server_fd), stopAccepting(stopAccepting) {}

signals:
    void newConnection(int socket);
    void errorOccurred(const QString &errorMsg);  

public slots:
    void startAccepting() {
        while (true) {
            if(!stopAccepting.load()){
            struct sockaddr_in address;
            int addrlen = sizeof(address);
            int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                continue;  
            }
            std::cout << "A new connection has been established" << std::endl;
            emit newConnection(new_socket);  
        }
        else{
            continue;

        }
    }}

    void stop() {
        std::cout << "Accepting connections has been stopped" << std::endl;
        stopAccepting.store(true);  
    }
    void start() {
        std::cout << "Accepting connections has been relaunched" << std::endl;
        stopAccepting.store(false);
}

private:
    int server_fd;
    std::atomic<bool>& stopAccepting;
};