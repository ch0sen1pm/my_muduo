// TcpServer.h — TCP 服务器顶层封装
//
// 组合 Acceptor + TcpConnection + EventLoopThreadPool，
// 提供多线程 Reactor 的完整 TCP 服务器。
//
// 使用示例：
//   TcpServer server(&loop, 8080);
//   server.setThreadNum(4);
//   server.setMessageCallback([](TcpConnection* conn, const char* data, size_t len) { ... });
//   server.start();
//   loop.loop();

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
    /// @param conn TcpConnection 指针（用于 send 回复）
    /// @param data 收到的数据
    /// @param len  数据长度
    using MessageCallback = std::function<void(TcpConnection* conn, const char* data, size_t len)>;

    /**
     * @param loop 主线程 EventLoop
     * @param port 监听端口号
     */
    TcpServer(EventLoop* loop, uint16_t port);
    ~TcpServer();

    /** 设置收到消息时的回调 */
    void setMessageCallback(MessageCallback cb);
    /** 启动服务器：启动线程池（如有）+ Acceptor 开始监听 */
    void start();
    /**
     * 设置工作线程数
     * @param numThreads 0 = 单线程模式，>0 = 多线程（one loop per thread）
     */
    void setThreadNum(int numThreads);

private:
    /** 构造 sockaddr_in 的辅助函数（内部使用） */
    static sockaddr_in makeAddr_(uint16_t port);
    /** Acceptor 回调：新连接到来 → new TcpConnection → 分配 loop */
    void onNewConnection(int connfd, const sockaddr_in& peer);

    EventLoop* loop_;
    Acceptor acceptor_;
    MessageCallback messageCallback_;
    uint16_t port_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;
};
