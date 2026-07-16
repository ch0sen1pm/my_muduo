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
    ssize_t n = inputBuffer_.readFd(channel_.fd());

    if (n > 0) {
        while (const char* crlf = inputBuffer_.findCRLF()) {
            size_t lineLen = crlf - inputBuffer_.peek();
            std::string msg = inputBuffer_.retrieveAsString(lineLen);
            inputBuffer_.retrieve(2);
            if (messageCallback_) {
                messageCallback_(msg.data(), msg.size());
            }
        }
    } else if (n == 0) {
        std::cout << "[TcpConnection] 客户端断开" << std::endl;
        channel_.disableAll();
    } else {
        std::cout << "[TcpConnection] 读错误" << std::endl;
        channel_.disableAll();
    }
}

void TcpConnection::send(const std::string& msg) {
    ::write(channel_.fd(), msg.data(), msg.size());
}