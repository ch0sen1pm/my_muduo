#include "Acceptor.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

Acceptor::Acceptor(EventLoop* loop, const sockaddr_in& listenAddr)
    : loop_(loop)
    , acceptSocket_(Socket::createTcp())
    , acceptChannel_(loop, acceptSocket_.fd())
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);

    acceptChannel_.setReadCallback([this]() {
        handleRead();
    });
}

Acceptor::~Acceptor() = default;

void Acceptor::setNewConnectionCallback(NewConnectionCallback cb) {
    newConnectionCallback_ = std::move(cb);
}

void Acceptor::listen() {
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    sockaddr_in peerAddr;
    memset(&peerAddr, 0, sizeof(peerAddr));
    socklen_t addrLen = sizeof(peerAddr);

    int connfd = ::accept(acceptSocket_.fd(),
                        (sockaddr*)&peerAddr,
                        &addrLen);

    if (connfd >= 0 && newConnectionCallback_) {
        newConnectionCallback_(connfd, peerAddr);
    } else if (connfd >= 0) {
        ::close(connfd);
    }
}