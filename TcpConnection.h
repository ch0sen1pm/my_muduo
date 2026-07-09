#pragma once
#include "Socket.h"
#include "Channel.h"
#include <functional>
#include "Buffer.h"

class EventLoop;

class TcpConnection {
public:
    using MessageCallback = std::function<void(const char* data, size_t len)>;

    TcpConnection(EventLoop* loop, int connfd);
    ~TcpConnection();

    void setMessageCallback(MessageCallback cb);

    void connectEstablished();

    void send(const std::string& msg);

private:
    void handleRead();

    EventLoop* loop_;
    Socket socket_;
    Channel channel_;
    MessageCallback messageCallback_;

    Buffer inputBuffer_;
};