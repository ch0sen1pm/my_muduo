#include "EventLoop.h"
#include "TcpServer.h"
#include <iostream>

int main() {
    EventLoop loop;
    TcpServer server(&loop, 8080);

    server.setMessageCallback([](const char* data, size_t len) {
        std::string msg(data, len);
        std::cout << "收到：" << msg << std::endl;
    });

    server.start();
    loop.loop();
}
