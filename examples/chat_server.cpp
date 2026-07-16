#include "net/TcpServer.h"
#include "net/TcpConnection.h"
#include "core/EventLoop.h"
#include "logger.h"
#include "core/crash_handler.h"
#include "core/rate_limiter.h"
#include <vector>
#include <iostream>


int main() {
    rate_limiter limiter(5);
    auto console = std::make_shared<color_stdout_sink>();
    auto file = std::make_shared<daily_rolling_sink>("chat.log");
    crash_handler::instance().add(file);

    auto log = std::make_shared<logger>("chat", console);
    log->add_sink(file);
    log->set_pattern("[%H:%M:%S] %v");
    log->set_level(level::info);
    
    EventLoop loop;
    TcpServer server(&loop, 8080);
    server.setThreadNum(4);
    std::vector<TcpConnection*> clients;

    server.setConnectionCallback([&](TcpConnection* conn) {
        clients.push_back(conn);
        LOG_ERROR(log, "新客户端链接，在线：" + std::to_string(clients.size()));
        conn->send("欢迎！在线人数：" + std::to_string(clients.size()) + "\r\n");
    });

    server.setMessageCallback([&](TcpConnection* conn, const char* data, size_t len) {
        std::string msg(data, len);

        for (int i = 0; i < 10; i ++) {
            if (limiter.should_log("test_repeat")) {
                LOG_INFO(log, "test_repeat");
            }
        }

        LOG_INFO(log, "收到：" + msg);

        if (msg == "crash") {
            int* p = nullptr;
            *p = 42;
        }

        for (auto* c : clients) {
            if (c != conn) {
                c->send(msg + "\r\n");   
            }
        }
    });

    LOG_INFO(log, "聊天服务器启动，端口 8080...");
    crash_handler::instance().install();
    server.start();
    loop.loop();
}