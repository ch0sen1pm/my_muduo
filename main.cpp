#include "core/EventLoop.h"
#include "net/TcpServer.h"
#include "net/TcpConnection.h"
#include "core/TimerQueue.h"
#include <iostream>

int main() {
    EventLoop loop;
    TcpServer server(&loop, 8080);
    server.setThreadNum(4);

    TimerQueue timer(&loop);
    timer.runAfter(3000, []() {
        std::cout << "[Timer] 3 秒到了！" << std::endl;
    });

    server.setMessageCallback([](TcpConnection* conn, const char* data, size_t len) {
        std::string msg(data, len);
        std::cout << "收到：" << msg << std::endl;
        conn->send("服务器收到：" + msg + "\r\n");
    });

    server.start();
    loop.loop();
}
