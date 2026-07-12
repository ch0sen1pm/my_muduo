// TcpConnection.h — 客户端连接管理
//
// 拥有连接 fd，负责读数据 / 发数据 / 回调通知。
// 跟 Acceptor 镜像：Acceptor 读 accept，TcpConnection 读数据。

#pragma once
#include "Socket.h"
#include "../core/Channel.h"
#include <functional>
#include "Buffer.h"

class EventLoop;

class TcpConnection {
public:
    /// @param data 收到的数据指针
    /// @param len  数据长度
    using MessageCallback = std::function<void(const char* data, size_t len)>;

    /**
     * @param loop   所属 EventLoop
     * @param connfd accept 返回的连接 fd
     */
    TcpConnection(EventLoop* loop, int connfd);
    ~TcpConnection();

    /** 设置收到数据时的回调 */
    void setMessageCallback(MessageCallback cb);

    /** 开始工作：将 Channel 注册到 epoll */
    void connectEstablished();

    /** 发送数据给客户端 */
    void send(const std::string& msg);

private:
    /** fd 可读 → read → 粘包处理 → 调 messageCallback_ */
    void handleRead();

    EventLoop* loop_;
    Socket socket_;               ///< RAII 持有连接 fd
    Channel channel_;             ///< 盯 fd 的可读事件
    MessageCallback messageCallback_;

    Buffer inputBuffer_;          ///< 输入缓冲区（readv + 按行切消息）
};
