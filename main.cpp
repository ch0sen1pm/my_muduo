// Socket demo: 测试 RAII fd 管理
// 创建 TCP socket → bind 端口 → listen → 挂到 EventLoop 上等连接
//
// nc 测试: nc localhost 8080  然后敲回车

#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main() {
    // 1. 创建非阻塞 TCP socket
    Socket listenSock = Socket::createTcp();
    std::cout << "[main] 创建 socket, fd = " << listenSock.fd() << std::endl;

    // 2. 允许端口复用
    listenSock.setReuseAddr(true);

    // 3. 绑定 0.0.0.0:8080
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0 = 监听所有网卡
    addr.sin_port = htons(8080);        // 端口 8080，htons = host to network short

    listenSock.bindAddress(addr);
    std::cout << "[main] 绑定 0.0.0.0:8080" << std::endl;

    // 4. 开始监听
    listenSock.listen();
    std::cout << "[main] 开始监听，nc localhost 8080 测试" << std::endl;

    // 5. 挂到 EventLoop 上，等客户端连接
    EventLoop loop;
    Channel listenChan(&loop, listenSock.fd());

    listenChan.setReadCallback([&]() {
        sockaddr_in peerAddr;
        memset(&peerAddr, 0, sizeof(peerAddr));
        socklen_t addrLen = sizeof(peerAddr);

        int connfd = ::accept(listenSock.fd(),
                              (sockaddr*)&peerAddr,
                              &addrLen);
        if (connfd >= 0) {
            char ipBuf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &peerAddr.sin_addr, ipBuf, sizeof(ipBuf));
            std::cout << "[main] 新连接! fd=" << connfd
                      << " ip=" << ipBuf
                      << " port=" << ntohs(peerAddr.sin_port) << std::endl;
            ::close(connfd);  // 演示用，直接关
        }
    });

    listenChan.enableReading();
    std::cout << "[main] 进入事件循环..." << std::endl;
    loop.loop();
}
