#include "TcpServer.h"
#include "TcpConnection.h"
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

sockaddr_in TcpServer::makeAddr_(uint16_t port) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    return addr;
}

TcpServer::TcpServer(EventLoop* loop, uint16_t port)
    : loop_(loop)
    , acceptor_(loop, makeAddr_(port))
    , port_(port)
{
    acceptor_.setNewConnectionCallback([this](int connfd, const sockaddr_in& peer) {
        onNewConnection(connfd, peer);
    });
}

TcpServer::~TcpServer() = default;

void TcpServer::setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
}

void TcpServer::onNewConnection(int connfd, const sockaddr_in& peer) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
    std::cout << "[TcpServer] 新连接！ fd=" << connfd
              << " ip=" << ip
              << " port=" << ntohs(peer.sin_port) << std::endl;

    EventLoop* ioLoop = threadPool_ ? threadPool_->getNextLoop() : loop_;
    auto* conn = new TcpConnection(ioLoop, connfd);
    conn->setMessageCallback([this, conn](const char* data, size_t len) {
        messageCallback_(conn, data, len);
    });
    conn->connectEstablished();

    if (connectionCallback_) {
        connectionCallback_(conn);
    }
}

void TcpServer::setThreadNum(int numThreads) {
    threadPool_ = std::make_unique<EventLoopThreadPool>(loop_, numThreads);
}

void TcpServer::start() {
    if (threadPool_) {
        threadPool_->start();
    }
    std::cout << "[TcpServer] 监听 " << port_ << "端口..." << std::endl;
    acceptor_.listen();
}

void TcpServer::setConnectionCallback(ConnectionCallback cb) {
    connectionCallback_ = std::move(cb);
}