#pragma once
#include "Socket.h"
#include "Channel.h"
#include <functional>
#include <netinet/in.h>

class EventLoop;

class Acceptor {
public:
    using NewConnectionCallback =
        std::function<void(int sockfd, const sockaddr_in& peerAddr)>;
    
    Acceptor(EventLoop* loop, const sockaddr_in& listenAddr);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb);
    void listen();

private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
};