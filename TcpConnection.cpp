#include "TcpConnection.h"
#include <unistd.h>
#include <iostream>

TcpConnection::TcpConnection(EventLoop* loop, int connfd)
    : loop_(loop)
    , socket_(connfd)
    , channel_(loop, connfd)
{
    channel_.setReadCallback([this]() {
        handleRead();
    });
}

TcpConnection::~TcpConnection() = default;

void TcpConnection::connectEstablished() {
    channel_.enableReading();
}

void TcpConnection::setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
}

void TcpConnection::handleRead() {
    char buf[4096];
    ssize_t n = ::read(channel_.fd(), buf, sizeof(buf));

    if (n > 0) {
        inputBuffer_.append(buf, n);
        if (messageCallback_) {
            messageCallback_(inputBuffer_.data(), inputBuffer_.size());
            inputBuffer_.clear();
        }
    } else if (n == 0) {
        std::cout << "[TcpConnection] 客户端断开" << std::endl;
    } else {
        std::cout << "[TcpConnection] 读错误" << std::endl;
    }
}

void TcpConnection::send(const std::string& msg) {
    ::write(channel_.fd(), msg.data(), msg.size());
}