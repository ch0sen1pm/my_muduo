#include "Socket.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

Socket::Socket(int sockfd)
    : sockfd_(sockfd)
{}

Socket::~Socket() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
    }
}

Socket::Socket(Socket&& other) noexcept
    : sockfd_(other.sockfd_)
{
    other.sockfd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (sockfd_ >= 0) {
            ::close(sockfd_);
        }
        sockfd_ = other.sockfd_;
        other.sockfd_ = -1;
    }

    return *this;
}

// AF_INET=IPv4, SOCK_STREAM=TCP
// SOCK_NONBLOCK: 非阻塞，epoll 必须。没有它 accept/read 会卡死整个线程
// SOCK_CLOEXEC:  exec 时自动关，防止子进程泄漏
Socket Socket::createTcp() {
    int fd = ::socket(AF_INET,
                    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                    0);
    
    if (fd < 0) {
        abort();
    }
    return Socket(fd);
}

void Socket::bindAddress(const sockaddr_in& addr) {
    int ret = ::bind(sockfd_,
                    (const sockaddr*)&addr,
                    sizeof(addr));

    if (ret < 0) {
        abort();
    }
}

// SOMAXCONN = 系统允许的全连接队列最大长度（Linux 下通常 128）
void Socket::listen() {
    int ret = ::listen(sockfd_, SOMAXCONN);
    if (ret < 0) {
        abort();
    }
}

// SO_REUSEADDR: 允许重用 TIME_WAIT 状态的端口。没它重启服务器会"Address already in use"
void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                &optval, sizeof(optval));
}