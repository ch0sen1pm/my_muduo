#include "EventLoop.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include <iostream>

int main() {
    EventLoop loop;
    TcpServer server(&loop, 8080);

    server.setMessageCallback([](TcpConnection* conn, const char* data, size_t len) {
        std::string msg(data, len);
        std::cout << "收到：" << msg << std::endl;
        conn->send("服务器收到：" + msg + "\r\n");
    });

    server.start();
    loop.loop();
}
