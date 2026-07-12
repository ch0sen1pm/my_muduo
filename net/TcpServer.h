#pragma once
#include <functional>
#include <string>
#include <netinet/in.h>
#include "Acceptor.h"
#include "../thread/EventLoopThreadPool.h"

class EventLoop;
class TcpConnection;

class TcpServer {
public:
    using MessageCallback = std::function<void(TcpConnection* conn, const char* data, size_t len)>;

    TcpServer(EventLoop* loop, uint16_t port);
    ~TcpServer();

    static sockaddr_in makeAddr_(uint16_t port);
    void setMessageCallback(MessageCallback cb);
    void start();
    void setThreadNum(int numThreads);

private:
    void onNewConnection(int connfd, const sockaddr_in& peer);

    EventLoop* loop_;
    Acceptor acceptor_;
    MessageCallback messageCallback_;
    uint16_t port_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;
};