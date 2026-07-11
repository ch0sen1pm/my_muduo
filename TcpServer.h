#pragma once
#include <functional>
#include <string>
#include <netinet/in.h>
#include "Acceptor.h"

class EventLoop;

class TcpServer {
public:
    using MessageCallback = std::function<void(const char* data, size_t len)>;

    TcpServer(EventLoop* loop, uint16_t port);
    ~TcpServer();

    static sockaddr_in makeAddr_(uint16_t port);
    void setMessageCallback(MessageCallback cb);
    void start();

private:
    void onNewConnection(int connfd, const sockaddr_in& peer);

    EventLoop* loop_;
    Acceptor acceptor_;
    MessageCallback messageCallback_;
    uint16_t port_;
};